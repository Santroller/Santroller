#include "tusb_option.h"
#include "devices/usb/host/xbox_wireless_host.h"
#include "devices/usb/host/xbox_wireless_controller.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "devices/usb.hpp"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"

extern "C"
{
#include "devices/usb/host/xbox/mt76.h"
#include "devices/usb/host/xbox/xbox_adapter.h"
#include "devices/usb/host/xbox/wireless.h"
#include "firmware_data.h"
#include "gip_button_mapping.h"
#include "gip_packet_handler.h"
#include "gip_device_mappings.h"
}

#include <algorithm>

namespace
{
constexpr uint16_t XBOX_WIRELESS_ADAPTER_VID = 0x045E;
constexpr uint16_t XBOX_WIRELESS_ADAPTER_PID_02E6 = 0x02E6;
constexpr uint16_t XBOX_WIRELESS_ADAPTER_PID_02FE = 0x02FE;
}

static XboxWirelessHost *wireless_host(struct mt76_dev *dev)
{
    return static_cast<XboxWirelessHost *>(dev ? dev->owner : nullptr);
}

extern "C" xbox_controller_t *xbox_get_controller(struct mt76_dev *dev, uint8_t index)
{
    auto host = wireless_host(dev);
    if (!host)
    {
        return nullptr;
    }

    auto controller = host->get_controller_interface(index);
    return controller ? controller->get_controller_data() : nullptr;
}

// The real subtype only arrives later via the GIP device descriptor.
extern "C" void xbox_create_controller(struct mt76_dev *dev, uint8_t index)
{
    auto host = wireless_host(dev);
    if (host)
    {
        host->create_controller_interface(index, SubType_Gamepad);
    }
}

extern "C" void xbox_remove_controller(struct mt76_dev *dev, uint8_t index)
{
    auto host = wireless_host(dev);
    if (host)
    {
        host->remove_controller_interface(index);
    }
}

int XboxWirelessHost::send_gip_to_controller(uint8_t wcid, const uint8_t *mac_addr, const uint8_t *data, uint16_t len)
{
    return mt76_send_gip_data(&m_mt76_dev, wcid, mac_addr, data, len);
}

extern "C" void xbox_adapter_process_gip_data(struct mt76_dev *dev, uint8_t wcid, const uint8_t *data, uint16_t len)
{
    auto host = wireless_host(dev);
    if (!host || wcid == 0)
    {
        if (!host) printf("xbox_adapter_process_gip_data: no host\n");
        if (wcid == 0) printf("xbox_adapter_process_gip_data: wcid=0\n");
        return;
    }

    if (len < 4)
    {
        printf("xbox_adapter_process_gip_data: wcid=%d, len=%d (too short)\n", wcid, len);
        return;
    }

    uint8_t controller_idx = wcid - 1;
    if (controller_idx >= XBOX_MAX_CONTROLLERS)
    {
        printf("xbox_adapter_process_gip_data: controller_idx=%d out of range\n", controller_idx);
        return;
    }

    auto controller = host->get_controller_interface(controller_idx);
    if (controller)
    {
        controller->process_gip_data(data, len);
    }
    else
    {
        printf("  No controller interface for idx %d\n", controller_idx);
    }
}

XboxWirelessHost::XboxWirelessHost(uint8_t dev_addr, uint8_t interface, uint16_t id)
    : UsbHostInterface(dev_addr, interface, id), m_adapter_initialized(false), m_firmware_loaded(false), m_firmware_loading(false), m_radio_initialized(false), m_pairing_initialized(false), m_last_update_time(0), m_init_start_time(0)
{
    m_delayed_init = true;
    m_controller_interfaces.fill(nullptr);
    m_gip_queue_head = 0;
    m_gip_queue_count = 0;

    printf("XboxWirelessHost: Created for dev_addr=%d, interface=%d\r\n", dev_addr, interface);
}

XboxWirelessHost::~XboxWirelessHost()
{
    for (auto &controller : m_controller_interfaces)
    {
        if (controller)
        {
            usb_host_remove_assignable_interface(controller.get());
            DeviceManager::instance().remove_device(controller.get());
        }
    }
    mt76_deinit(&m_mt76_dev);

    printf("XboxWirelessHost: Destroyed\r\n");
}

std::shared_ptr<UsbHostInterface> XboxWirelessHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    uint32_t size = desc_itf->bLength;

    TU_VERIFY(desc_itf->bInterfaceNumber == 0, nullptr);

    uint8_t dev_addr = list->dev_addr();
    uint16_t vid;
        uint16_t pid;
        TU_VERIFY(tuh_vid_pid_get(dev_addr, &vid, &pid), nullptr);
        TU_VERIFY(vid == XBOX_WIRELESS_ADAPTER_VID &&
                  (pid == XBOX_WIRELESS_ADAPTER_PID_02E6 || pid == XBOX_WIRELESS_ADAPTER_PID_02FE),
                  nullptr);

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XboxWirelessHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    intf->set_subtype(SubType_Gamepad);
    intf->m_adapter_pid = pid;
    uint8_t endpoints = desc_itf->bNumEndpoints;
    printf("XboxWirelessHost: Parsing %d endpoints\r\n", endpoints);

    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
        size += desc_ep->bLength;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);

        if (desc_ep->bEndpointAddress & 0x80)
        {
            printf("XboxWirelessHost: Bulk IN endpoint 0x%02X, size=%d\r\n",
                   desc_ep->bEndpointAddress, desc_ep->wMaxPacketSize);
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
                 list->host_devices_by_endpoint_in[desc_ep->bEndpointAddress & (~0x80)] = intf;
            if (desc_ep->bEndpointAddress == MT_EP_IN_CMD)
            {
                usbh_edpt_xfer(dev_addr, desc_ep->bEndpointAddress, intf->m_cmd_buf, sizeof(intf->m_cmd_buf));
            }
            else if (desc_ep->bEndpointAddress == MT_EP_IN_WLAN)
            {
                usbh_edpt_xfer(dev_addr, desc_ep->bEndpointAddress, intf->m_data_buf, sizeof(intf->m_data_buf));
            }
        }
        else
        {
            printf("XboxWirelessHost: Bulk OUT endpoint 0x%02X, size=%d\r\n",
                   desc_ep->bEndpointAddress, desc_ep->wMaxPacketSize);
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
            list->host_devices_by_endpoint_out[desc_ep->bEndpointAddress] = intf;
        }
    }

    usb_host_add_enumerating_interface(intf);

    printf("XboxWirelessHost: Interface opened, size=%d\r\n", size);
    *out_len = size;
    return intf;
}

bool XboxWirelessHost::set_config()
{
    printf("XboxWirelessHost::set_config\r\n");
    UsbHostInterface::set_config();

    initialize_adapter();

    return true;
}

void XboxWirelessHost::initialize_adapter()
{
    if (m_adapter_initialized)
    {
        return;
    }

    printf("XboxWirelessHost: Initializing MT76 adapter\r\n");
    m_init_start_time = to_ms_since_boot(get_absolute_time());

    if (!mt76_init(&m_mt76_dev, m_dev_addr))
    {
        printf("XboxWirelessHost: MT76 init failed\r\n");
        return;
    }

    m_mt76_dev.owner = this;
    m_mt76_dev.pending_pairing = -1;

    m_adapter_initialized = true;

    printf("XboxWirelessHost: MT76 adapter initialized and ready\r\n");
}

void XboxWirelessHost::create_controller_interface(uint8_t controller_idx, uint8_t subtype)
{
    if (controller_idx >= XBOX_MAX_CONTROLLERS)
    {
        return;
    }

    if (m_controller_interfaces[controller_idx])
    {
        printf("XboxWirelessHost: Controller %d interface already exists\r\n", controller_idx);
        return;
    }

    printf("XboxWirelessHost: Creating virtual interface for controller %d (subtype %d)\r\n",
           controller_idx, subtype);

    auto controller_intf = std::make_shared<XboxWirelessController>(
        this, controller_idx, m_dev_addr, m_id);

    controller_intf->set_subtype((SubType)subtype);

    m_controller_interfaces[controller_idx] = controller_intf;

    printf("XboxWirelessHost: Controller %d virtual interface created and assignable\r\n",
           controller_idx);
}

std::shared_ptr<XboxWirelessController> XboxWirelessHost::get_controller_interface(uint8_t controller_idx)
{
    if (controller_idx >= XBOX_MAX_CONTROLLERS)
    {
        return nullptr;
    }

    return m_controller_interfaces[controller_idx];
}

void XboxWirelessHost::remove_controller_interface(uint8_t controller_idx)
{
    if (controller_idx >= XBOX_MAX_CONTROLLERS)
    {
        return;
    }

    auto controller_intf = m_controller_interfaces[controller_idx];
    if (!controller_intf)
    {
        return;
    }

    printf("XboxWirelessHost: Removing virtual interface for controller %d\r\n", controller_idx);

    usb_host_remove_assignable_interface(controller_intf.get());
    DeviceManager::instance().remove_device(controller_intf.get());
    clear_queued_packets(controller_idx + 1);

    m_controller_interfaces[controller_idx].reset();

    printf("XboxWirelessHost: Controller %d virtual interface removed\r\n", controller_idx);
}

bool XboxWirelessHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr == MT_EP_IN_CMD && result != XFER_RESULT_FAILED)
    {
        if (xferred_bytes > 0)
        {
            wireless_process_data(&m_mt76_dev, m_cmd_buf, xferred_bytes);
        }

        usbh_edpt_xfer(m_dev_addr, ep_addr, m_cmd_buf, sizeof(m_cmd_buf));
    }
    else if (ep_addr == MT_EP_IN_WLAN && result != XFER_RESULT_FAILED)
    {
        if (xferred_bytes > 0)
        {
            wireless_process_data(&m_mt76_dev, m_data_buf, xferred_bytes);
        }

        usbh_edpt_xfer(m_dev_addr, ep_addr, m_data_buf, sizeof(m_data_buf));
    }
    else if (ep_addr == MT_EP_IN_CMD || ep_addr == MT_EP_IN_WLAN)
    {
        usbh_edpt_xfer(m_dev_addr, ep_addr,
                       ep_addr == MT_EP_IN_CMD ? m_cmd_buf : m_data_buf,
                       ep_addr == MT_EP_IN_CMD ? sizeof(m_cmd_buf) : sizeof(m_data_buf));
    }

    return true;
}

bool XboxWirelessHost::queue_gip_packet(uint8_t wcid, const uint8_t *mac_addr, const uint8_t *packet, uint16_t len, bool priority)
{
    if (!packet || !mac_addr || len == 0 || len > GIP_REPORT_QUEUE_MAX_SIZE ||
        m_gip_queue_count >= WIRELESS_GIP_QUEUE_CAPACITY)
    {
        return false;
    }

    if (priority)
    {
        m_gip_queue_head = (m_gip_queue_head + WIRELESS_GIP_QUEUE_CAPACITY - 1) % WIRELESS_GIP_QUEUE_CAPACITY;
    }
    uint8_t index = priority ? m_gip_queue_head :
        (m_gip_queue_head + m_gip_queue_count) % WIRELESS_GIP_QUEUE_CAPACITY;
    auto &item = m_gip_queue[index];
    memcpy(item.packet, packet, len);
    memcpy(item.mac_addr, mac_addr, MT76_MAC_ADDR_LEN);
    item.len = len;
    item.wcid = wcid;
    m_gip_queue_count++;
    return true;
}

void XboxWirelessHost::send_report_from_host(uint8_t wcid, const uint8_t *mac_addr, const uint8_t *packet, uint16_t len)
{
    queue_gip_packet(wcid, mac_addr, packet, len, false);
}

void XboxWirelessHost::send_ack_from_host(uint8_t wcid, const uint8_t *mac_addr, const uint8_t *packet, uint16_t len)
{
    queue_gip_packet(wcid, mac_addr, packet, len, true);
}

void XboxWirelessHost::clear_queued_packets(uint8_t wcid)
{
    if (m_gip_queue_count == 0)
    {
        return;
    }

    uint8_t remaining_count = 0;
    for (uint8_t i = 0; i < m_gip_queue_count; i++)
    {
        uint8_t index = (m_gip_queue_head + i) % WIRELESS_GIP_QUEUE_CAPACITY;
        if (m_gip_queue[index].wcid != wcid)
        {
            uint8_t destination = (m_gip_queue_head + remaining_count) % WIRELESS_GIP_QUEUE_CAPACITY;
            if (destination != index)
            {
                m_gip_queue[destination] = m_gip_queue[index];
            }
            remaining_count++;
        }
    }

    m_gip_queue_head = 0;
    m_gip_queue_count = remaining_count;
}

void XboxWirelessHost::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);

    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (!m_adapter_initialized)
    {
        return;
    }

    if (!m_firmware_loaded && (now - m_init_start_time) > 500)
    {
        const uint8_t *firmware_data;
        uint32_t firmware_len;
        uint32_t firmware_compressed_len;

        if (m_adapter_pid == XBOX_WIRELESS_ADAPTER_PID_02FE)
        {
            firmware_data = mt76_firmware_02fe;
            firmware_len = mt76_firmware_02fe_len;
            firmware_compressed_len = mt76_firmware_02fe_compressed_len;
        }
        else
        {
            firmware_data = mt76_firmware_02e6;
            firmware_len = mt76_firmware_02e6_len;
            firmware_compressed_len = mt76_firmware_02e6_compressed_len;
        }

        if (!m_firmware_loading)
        {
            if (mt76_begin_firmware_compressed(&m_mt76_dev, firmware_data,
                                               firmware_compressed_len, firmware_len) < 0)
            {
                printf("XboxWirelessHost: Firmware decompression failed\r\n");
                return;
            }
            m_firmware_loading = true;
        }

        int stream_result = mt76_step_firmware_compressed(&m_mt76_dev);
        if (stream_result > 0)
        {
            m_last_update_time = now;
            return;
        }
        if (stream_result < 0)
        {
            m_firmware_loading = false;
            printf("XboxWirelessHost: Firmware decompression failed\r\n");
            return;
        }

        printf("XboxWirelessHost: Firmware loaded successfully\r\n");
        m_firmware_loading = false;
        m_firmware_loaded = true;

        if (mt76_load_ivb(&m_mt76_dev) == 0)
        {
            printf("XboxWirelessHost: IVB loaded successfully\r\n");
        }
    }

    if (m_firmware_loaded && !m_radio_initialized && (now - m_init_start_time) > 2000)
    {
        printf("XboxWirelessHost: Initializing radio\r\n");

        if (mt76_init_radio(&m_mt76_dev) == 0)
        {
            printf("XboxWirelessHost: Radio initialized successfully\r\n");
            m_radio_initialized = true;

            if (mt76_init_channels(&m_mt76_dev) == 0)
            {
                printf("XboxWirelessHost: Channels initialized\r\n");
            }
        }
        else
        {
            printf("XboxWirelessHost: Radio init failed\r\n");
        }
    }

    if (m_radio_initialized && !m_pairing_initialized && (now - m_init_start_time) > 3000)
    {
        printf("XboxWirelessHost: Disabling pairing mode\r\n");

        if (mt76_set_pairing(&m_mt76_dev, false) == 0)
        {
            printf("XboxWirelessHost: Pairing mode disabled\r\n");
            m_pairing_initialized = true;
        }
    }

    if (m_pairing_initialized)
    {
        wireless_task(&m_mt76_dev);
    }
    if (m_gip_queue_count > 0)
    {
        auto &item = m_gip_queue[m_gip_queue_head];
        if (mt76_send_gip_data(&m_mt76_dev, item.wcid, item.mac_addr, item.packet, item.len) == 0)
        {
            m_gip_queue_head = (m_gip_queue_head + 1) % WIRELESS_GIP_QUEUE_CAPACITY;
            m_gip_queue_count--;
        }
    }

    m_last_update_time = now;
}

bool XboxWirelessHost::tick_digital(proto_Output &type)
{
    return false;
}

uint16_t XboxWirelessHost::tick_analog(proto_Output &type)
{
    return 0;
}
