#include "devices/usb.hpp"
#include "tusb.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "usb/host/xinput_host.h"
#include "usb/host/hid_host.h"
#include "usb/host/midi_host.h"
#include "config.hpp"
#include "usb/device//hid_device.h"
#include <algorithm>
#include "utils.h"
static uint8_t usb_host_id;
USBHostHardwareDevice::USBHostHardwareDevice(proto_UsbHostDevice device, uint16_t id) : UsbHostInterface(0, 0, id), m_device(device)
{
    printf("UsbHostHardwareDevice: %p\r\n", this);
    if (device.firstPin == -1)
    {
        return;
    }
    pio_usb_configuration_t host_config = {
        pin_dp : (uint8_t)(device.firstPin + device.dmFirst),
        pio_tx_num : 0,
        sm_tx : 0,
        tx_ch : 0,
        pio_rx_num : 0,
        sm_rx : 1,
        sm_eop : 2,
        alarm_pool : NULL,
        debug_pin_rx : -1,
        debug_pin_eop : -1,
        skip_alarm_pool : false,
        pinout : device.dmFirst ? PIO_USB_PINOUT_DMDP : PIO_USB_PINOUT_DPDM
    };
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_HOST,
        .speed = TUH_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL,
    };
    usb_host_id = id;
    tuh_configure(TUH_OPT_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &host_config);
    tusb_init(TUH_OPT_RHPORT, &rh_init);
    printf("assignable_devices before: %d\r\n", assignable_usb_devices.size());

    for (auto it = assignable_usb_devices.begin(); it != assignable_usb_devices.end();)
    {
        if (!tuh_connected(it[0]->dev_addr()))
        {
            it = assignable_usb_devices.erase(it);
        }
        else
        {
            assignable_devices.push_back(it[0]);
            it[0]->rescan(true);
            ++it;
        }
    }
    printf("assignable_devices after: %d\r\n", assignable_usb_devices.size());
}

void USBHostHardwareDevice::update(bool full_poll, bool send_events)
{
}
void USBHostHardwareDevice::rescan(bool first)
{
    printf("usbhosthardware rescan\r\n");
}

bool USBHostHardwareDevice::using_pin(uint8_t pin)
{
    return pin == m_device.firstPin || pin == m_device.firstPin + 1;
}
std::shared_ptr<UsbHostDevice> host_devices[127];
bool usbh_init(void)
{
    printf("usbh init\r\n");
    for (size_t i = 0; i < TU_ARRAY_SIZE(host_devices); i++)
    {
        host_devices[i] = nullptr;
    }
    return true;
}
void process_product_string(tuh_xfer_t *xfer)
{
    if (host_devices[xfer->daddr] != nullptr)
    {
        // send state change event
        for (auto &device : host_devices[xfer->daddr]->host_devices_by_itf)
        {
            if (device != nullptr)
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
    tuh_descriptor_get_product_string(m_dev_addr, 0x0409, m_name, sizeof(m_name), process_product_string, 0);
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
    HidHost::open,
    MidiHost::open};

uint16_t usbh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    printf("usbh open %d %d\r\n", dev_addr, desc_itf->bInterfaceNumber);
    if (host_devices[dev_addr] == nullptr)
    {
        host_devices[dev_addr] = std::make_shared<UsbHostDevice>(dev_addr, usb_host_id);
    }
    for (auto &host_device : host_device_types)
    {
        uint16_t out_len;
        auto dev = host_device(host_devices[dev_addr], desc_itf, max_len, &out_len);
        if (dev != nullptr)
        {
            host_devices[dev_addr]->host_devices_by_itf[desc_itf->bInterfaceNumber] = dev;
            return out_len;
        }
    }
    return 0;
}

bool usbh_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    if (host_devices[dev_addr]->host_devices_by_itf[itf_num] == nullptr)
    {
        return true;
    }
    if (HIDConfigDevice::tool_closed())
    {
        reload();
    }
    return host_devices[dev_addr]->host_devices_by_itf[itf_num]->set_config();
}

bool usbh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (host_devices[dev_addr] == nullptr)
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
    if (dev == nullptr)
    {
        return false;
    }
    return dev->xfer_cb(ep_addr, result, xferred_bytes);
}

void usbh_close(uint8_t dev_addr)
{
    printf("usbh close %d %d\r\n", dev_addr);
    if (host_devices[dev_addr] != nullptr)
    {
        host_devices[dev_addr]->disconnect();
    }
    host_devices[dev_addr] = nullptr;
    if (HIDConfigDevice::tool_closed())
    {
        reload();
    }
}

void UsbHostDevice::disconnect()
{
    for (auto &device : host_devices_by_itf)
    {
        if (device != nullptr) {
            device->disconnect();
        }
    }
}

void UsbHostInterface::disconnect() {
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