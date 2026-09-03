#include "devices/usb.hpp"
#include "managers/device_manager.hpp"

#include "usb/auth_broker.h"
#include "tusb.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "devices/usb/host/xinput_host.h"
#include "devices/usb/host/hid/hid_host.h"
#include "devices/usb/host/midi_host.h"
#include "devices/usb/host/ogxbox_host.h"
#include "devices/usb/host/xone_host.h"
#include "devices/usb/host/xbox_wireless_host.h"
#include "hardware/pio.h"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include "hardware/dma.h"
#include <algorithm>
#include <vector>
#include "utils.h"
static uint8_t usb_host_id;
static bool m_initialized = false;
static int8_t m_last_first_pin = -1;
static bool m_last_dp_first = false;
static volatile uint32_t m_devices_changed = 0;
std::array<std::shared_ptr<UsbHostDevice>, 127> host_devices;

static std::vector<std::shared_ptr<UsbHostInterface>> usb_assignable_interfaces;
static std::vector<std::shared_ptr<UsbHostInterface>> usb_enumerating_interfaces;

template <typename Predicate>
static void erase_usb_interfaces_if(std::vector<std::shared_ptr<UsbHostInterface>> &interfaces, Predicate predicate)
{
    interfaces.erase(std::remove_if(interfaces.begin(), interfaces.end(), predicate), interfaces.end());
}

void usb_host_add_assignable_interface(std::shared_ptr<UsbHostInterface> device)
{
    usb_assignable_interfaces.push_back(device);
}

void usb_host_add_enumerating_interface(std::shared_ptr<UsbHostInterface> device)
{
    usb_enumerating_interfaces.push_back(device);
}

void usb_host_remove_assignable_interface(UsbHostInterface *device)
{
    erase_usb_interfaces_if(usb_assignable_interfaces, [device](const auto &candidate) { return candidate.get() == device; });
}

void usb_host_remove_enumerating_interface(UsbHostInterface *device)
{
    erase_usb_interfaces_if(usb_enumerating_interfaces, [device](const auto &candidate) { return candidate.get() == device; });
}

void usb_host_remove_interfaces_by_address(uint8_t dev_addr)
{
    erase_usb_interfaces_if(usb_assignable_interfaces, [dev_addr](const auto &device) { return device->dev_addr() == dev_addr; });
    erase_usb_interfaces_if(usb_enumerating_interfaces, [dev_addr](const auto &device) { return device->dev_addr() == dev_addr; });
}

size_t usb_host_assignable_interface_count()
{
    return usb_assignable_interfaces.size();
}

void usb_host_add_assignable_devices(bool rescan)
{
    for (const auto &device : usb_assignable_interfaces)
    {
        DeviceManager::instance().add_assignable_device(device);
        if (rescan)
        {
            device->rescan(true);
        }
    }
}

void usb_host_update_interfaces(bool full_poll, bool send_events)
{
    for (const auto &device : usb_enumerating_interfaces)
    {
        device->update(full_poll, send_events);
    }
    for (const auto &device : usb_assignable_interfaces)
    {
        device->update(full_poll, send_events);
    }
}

void process_delayed_init()
{
    m_devices_changed = millis() + 500;
}
USBHostHardwareDevice::USBHostHardwareDevice(proto_UsbHostDevice device, uint16_t id) : UsbHostInterface(0, 0, id), m_device(device)
{
    printf("UsbHostHardwareDevice: %p\r\n", this);
}

USBHostHardwareDevice::~USBHostHardwareDevice()
{
    printf("USBHostHardwareDevice destructor\r\n");
}
void USBHostHardwareDevice::begin()
{
    printf("USBHostHardwareDevice begin! %d %d %d %d\r\n", m_initialized, m_device.dmFirst, m_device.firstPin, m_device.firstPin + 1);
    if (m_device.firstPin == -1)
    {
        return;
    }
    if (m_initialized)
    {
        if (m_last_dp_first == m_device.dmFirst && m_last_first_pin == m_device.firstPin)
        {
            printf("usbhost init already done\r\n");
            return;
        }
        // pins changed, need to deinit
        end(true);
    }

    m_last_dp_first = m_device.dmFirst;
    m_last_first_pin = m_device.firstPin;
    printf("USBHostHardwareDevice init! %d %d %d\r\n", m_device.dmFirst, m_device.firstPin, m_device.firstPin + 1);
    m_initialized = true;
    int8_t tx_ch = -1;
    int8_t tx_sm = -1;
    int8_t rx_sm = -1;
    int8_t eop_sm = -1;
    tx_ch = dma_claim_unused_channel(false);
    tx_sm = pio_claim_unused_sm(pio0, true);
    rx_sm = pio_claim_unused_sm(pio0, true);
    eop_sm = pio_claim_unused_sm(pio0, true);
    pio_usb_configuration_t host_config = {
        pin_dp : (uint8_t)(m_device.firstPin + m_device.dmFirst),
        pio_tx_num : 0,
        sm_tx : (uint8_t)tx_sm,
        tx_ch : (uint8_t)tx_ch,
        pio_rx_num : 0,
        sm_rx : (uint8_t)rx_sm,
        sm_eop : (uint8_t)eop_sm,
        alarm_pool : NULL,
        debug_pin_rx : -1,
        debug_pin_eop : -1,
        skip_alarm_pool : false,
        pinout : m_device.dmFirst ? PIO_USB_PINOUT_DMDP : PIO_USB_PINOUT_DPDM
    };
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUH_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL,
    };
    usb_host_id = m_id;
    tuh_configure(TUH_OPT_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &host_config);
    dma_channel_unclaim(tx_ch);
    pio_sm_unclaim(pio0, tx_sm);
    pio_sm_unclaim(pio0, rx_sm);
    pio_sm_unclaim(pio0, eop_sm);
    tusb_init(TUH_OPT_RHPORT, &rh_init);
}
void USBHostHardwareDevice::end(bool full)
{
    if (full)
    {
        printf("usbhost deinit\r\n");
        tusb_deinit(TUH_OPT_RHPORT);
    }
}

void USBHostHardwareDevice::update(bool full_poll, bool send_events)
{
    if (m_devices_changed && millis() > m_devices_changed)
    {
        printf("devices changed! count: %d\r\n", usb_host_assignable_interface_count());
        m_devices_changed = 0;
        reload();
    }
    if (full_poll)
    {

        printf("usbhosthardware update %d %d\r\n", full_poll, send_events);
    }
    usb_host_update_interfaces(full_poll, send_events);
}
void USBHostHardwareDevice::rescan(bool first)
{
    printf("usbhosthardware rescan\r\n");
    if (first)
    {
        printf("assignable_devices before: %d\r\n", usb_host_assignable_interface_count());

        usb_host_add_assignable_devices(true);
        printf("assignable_devices after: %d\r\n", usb_host_assignable_interface_count());
    }
}

bool USBHostHardwareDevice::using_pin(uint8_t pin)
{
    return pin == m_device.firstPin || pin == m_device.firstPin + 1;
}
bool usbh_init(void)
{
    printf("usbh init\r\n");
    return true;
}

void process_product_string(tuh_xfer_t *xfer)
{
    if (host_devices[xfer->daddr])
    {
        // send state change event
        for (auto &device : host_devices[xfer->daddr]->host_devices_by_itf)
        {
            if (device)
            {
                device->update(true, true);
            }
        }
    }
}
bool UsbHostInterface::set_config()
{
    // TODO: get lang id and use it
    // also, deal with devices that dont have product names
    if (m_has_name)
    {
        update(true, true);
    }
    else
    {
        tuh_descriptor_get_product_string(m_dev_addr, 0x0409, m_name, sizeof(m_name), process_product_string, (uintptr_t)this);
    }
    usbh_driver_set_config_complete(m_dev_addr, m_interface);
    if (!m_delayed_init)
    {
        m_devices_changed = millis() + 500;
    }
    return true;
}

void UsbHostInterface::update(bool full_poll, bool send_events)
{
    MidiDevice::update(full_poll, send_events);
    if (send_events && full_poll)
    {
        m_sent_type = true;
        proto_Event event = {which_event : proto_Event_usb_tag, event : {usb : {m_id, m_subtype, m_dev_addr, m_interface, true}}};
        for (size_t i = 0; i < sizeof(event.event.usb.name); i++)
        {
            // skip header
            event.event.usb.name[i] = m_name[(i + 1) * 2];
        }
        HIDConfigDevice::send_event(event, true);
    }
}

bool UsbHostInterface::send_intr_xfer(uint8_t endpoint, const void *buffer, uint8_t len)
{
    TU_VERIFY(usbh_edpt_claim(m_dev_addr, endpoint));
    if (!usbh_edpt_xfer(m_dev_addr, endpoint, (uint8_t *)buffer, len))
    {
        usbh_edpt_release(m_dev_addr, endpoint);
        return false;
    }
    return true;
}

uint32_t UsbHostInterface::send_ctrl_xfer(tusb_control_request_t setup, void *buffer, bool *status)
{
    tuh_xfer_t xfer = {};
    xfer.daddr = m_dev_addr;
    xfer.ep_addr = 0;
    xfer.setup = &setup;
    xfer.buffer = (uint8_t *)buffer;
    xfer.complete_cb = NULL;
    xfer.user_data = 0;
    tuh_control_xfer(&xfer);
    if (xfer.result != XFER_RESULT_SUCCESS)
    {
        if (status)
        {
            *status = false;
        }
        return false;
    }
    return xfer.actual_len;
}

static std::shared_ptr<UsbHostInterface> (*host_device_types[])(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t *out_len) = {
    XInputGamepadHost::open,
    XInputAudioHost::open,
    XInputModuleHost::open,
    XInputSecurityHost::open,
    XInputBigButtonHost::open,
    XInputWirelessGamepadHost::open,
    XInputWirelessAudioHost::open,
    OGXboxHost::open,
    XboxOneHost::open,
    XboxWirelessHost::open,
    HidHost::open,
    MidiHost::open};

uint16_t usbh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    if (!host_devices[dev_addr])
    {
        host_devices[dev_addr] = std::make_shared<UsbHostDevice>(dev_addr, usb_host_id);
    }
    if (TUSB_CLASS_HUB == desc_itf->bInterfaceClass)
    {
        return 0;
    }
    printf("trying to open\r\n");
    for (auto &host_device : host_device_types)
    {
        uint16_t out_len;
        auto dev = host_device(host_devices[dev_addr], desc_itf, max_len, &out_len);
        if (dev)
        {
            host_devices[dev_addr]->host_devices_by_itf[desc_itf->bInterfaceNumber] = dev;
            printf("done\r\n");
            return out_len;
        }
    }
    return 0;
}

bool usbh_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    if (!host_devices[dev_addr] || !host_devices[dev_addr]->host_devices_by_itf[itf_num])
    {
        return false;
    }
    return host_devices[dev_addr]->host_devices_by_itf[itf_num]->set_config();
}

bool usbh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (!host_devices[dev_addr])
    {
        return false;
    }
    std::shared_ptr<UsbHostInterface> dev;
    if (ep_addr & 0x80)
    {
        dev = host_devices[dev_addr]->host_devices_by_endpoint_in[ep_addr & (~0x80)];
    }
    else
    {
        dev = host_devices[dev_addr]->host_devices_by_endpoint_out[ep_addr];
    }
    if (!dev)
    {
        return false;
    }
    return dev->xfer_cb(ep_addr, result, xferred_bytes);
}

void usbh_close(uint8_t dev_addr)
{
    printf("usbh close %d %d\r\n", dev_addr);
    if (host_devices[dev_addr])
    {
        host_devices[dev_addr]->disconnect();
        for (auto &device : host_devices[dev_addr]->host_devices_by_itf)
        {
            DeviceManager::instance().remove_device(device.get());
        }
        for (auto &device : host_devices[dev_addr]->host_devices_by_endpoint_in)
        {
            DeviceManager::instance().remove_device(device.get());
        }
        for (auto &device : host_devices[dev_addr]->host_devices_by_endpoint_out)
        {
            DeviceManager::instance().remove_device(device.get());
        }
        usb_host_remove_interfaces_by_address(dev_addr);
        m_devices_changed = millis() + 500;
        host_devices[dev_addr] = nullptr;
    }
    // Auth handlers are automatically unregistered by auth_broker when needed
}

void UsbHostDevice::disconnect()
{
    printf("disconnect\r\n");
    for (auto &device : host_devices_by_itf)
    {
        if (device)
        {
            device->disconnect();
        }
    }
}

void UsbHostInterface::disconnect()
{
    proto_Event event = {which_event : proto_Event_usb_tag, event : {usb : {m_id, SubType_Gamepad, m_dev_addr, m_interface, false}}};
    HIDConfigDevice::send_event(event, true);
}

static const usbh_class_driver_t driver_host[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "Santroller_Host",
#endif
        .init = usbh_init,
        .open = usbh_open,
        .set_config = usbh_set_config,
        .xfer_cb = usbh_xfer_cb,
        .close = usbh_close}};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = TU_ARRAY_SIZE(driver_host);
    return driver_host;
}