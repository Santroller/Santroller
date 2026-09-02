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
#include "devices/usb/host/xbox/firmware_decompress.h"
#include "firmware_data.h"
#include "gip_button_mapping.h"
#include "gip_packet_handler.h"
#include "gip_device_mappings.h"
}

#include <algorithm>

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
    if (!host || len < 4 || wcid == 0)
    {
        return;
    }

    uint8_t controller_idx = wcid - 1;
    if (controller_idx >= XBOX_MAX_CONTROLLERS)
    {
        return;
    }

    auto controller = host->get_controller_interface(controller_idx);
    if (controller)
    {
        controller->process_gip_data(data, len);
    }
}

XboxWirelessHost::XboxWirelessHost(uint8_t dev_addr, uint8_t interface, uint16_t id)
    : UsbHostInterface(dev_addr, interface, id), m_adapter_initialized(false), m_firmware_loaded(false), m_radio_initialized(false), m_pairing_initialized(false), m_last_update_time(0), m_init_start_time(0)
{
    m_delayed_init = true;
    m_controller_interfaces.fill(nullptr);
    m_report_queue = gip_report_queue_create();

    printf("XboxWirelessHost: Created for dev_addr=%d, interface=%d\r\n", dev_addr, interface);
}

XboxWirelessHost::~XboxWirelessHost()
{
    gip_report_queue_destroy(m_report_queue);
    mt76_deinit(&m_mt76_dev);

    printf("XboxWirelessHost: Destroyed\r\n");
}

std::shared_ptr<UsbHostInterface> XboxWirelessHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    uint32_t size = desc_itf->bLength;

    TU_VERIFY(desc_itf->bInterfaceNumber == 0, nullptr);

    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XboxWirelessHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    intf->set_subtype(SubType_Gamepad);

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

    DeviceManager::instance().add_enumerating_usb_device(intf);

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

    DeviceManager::instance().remove_assignable_usb_device(controller_intf.get());

    m_controller_interfaces[controller_idx].reset();

    printf("XboxWirelessHost: Controller %d virtual interface removed\r\n", controller_idx);
}

bool XboxWirelessHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    printf("XboxWirelessHost: RX endpoint 0x%02X, result=%d, bytes=%lu\r\n",
           ep_addr, result, xferred_bytes);

    if ((ep_addr == MT_EP_IN_CMD || ep_addr == MT_EP_IN_WLAN) &&
        result == XFER_RESULT_SUCCESS && xferred_bytes >= 8)
    {
        printf("XboxWirelessHost: RX header %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[0] : m_data_buf[0],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[1] : m_data_buf[1],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[2] : m_data_buf[2],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[3] : m_data_buf[3],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[4] : m_data_buf[4],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[5] : m_data_buf[5],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[6] : m_data_buf[6],
               ep_addr == MT_EP_IN_CMD ? m_cmd_buf[7] : m_data_buf[7]);
    }

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

void XboxWirelessHost::send_report_from_host(uint8_t *packet, uint16_t len)
{
    gip_report_queue_push(m_report_queue, packet, len);
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
        printf("XboxWirelessHost: Loading firmware\r\n");

        const uint8_t *firmware_data = mt76_firmware_02e6;
        uint32_t firmware_len = mt76_firmware_02e6_len;
        uint8_t *firmware_decompressed = NULL;

#if mt76_firmware_02e6_is_compressed
        printf("Decompressing firmware (%u bytes compressed -> %u bytes)\r\n",
               mt76_firmware_02e6_compressed_len, mt76_firmware_02e6_len);
        firmware_decompressed = decompress_firmware(firmware_data,
                                                    mt76_firmware_02e6_compressed_len,
                                                    mt76_firmware_02e6_len);
        if (firmware_decompressed)
        {
            firmware_data = firmware_decompressed;
        }
        else
        {
            printf("XboxWirelessHost: Firmware decompression failed\r\n");
            return;
        }
#endif

        if (mt76_load_firmware(&m_mt76_dev, firmware_data, firmware_len) == 0)
        {
            printf("XboxWirelessHost: Firmware loaded successfully\r\n");
            m_firmware_loaded = true;

            if (mt76_load_ivb(&m_mt76_dev) == 0)
            {
                printf("XboxWirelessHost: IVB loaded successfully\r\n");
            }
        }
        else
        {
            printf("XboxWirelessHost: Firmware load failed\r\n");
        }

        if (firmware_decompressed)
        {
            free(firmware_decompressed);
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
    if (!gip_report_queue_empty(m_report_queue))
    {
        const gip_report_queue_item_t *item = gip_report_queue_front(m_report_queue);
        if (item && mt76_usb_bulk_out(&m_mt76_dev, item->report, item->len) == 0)
        {
            gip_report_queue_pop(m_report_queue);
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
