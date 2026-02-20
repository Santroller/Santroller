#include "devices/usb.hpp"
#include "tusb.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "usb/host/xinput_host.h"
#include "config.hpp"
std::shared_ptr<USBHostHardwareDevice> USBHostHardwareDevice::instance = nullptr;
USBHostHardwareDevice::USBHostHardwareDevice(proto_UsbHostDevice device, uint16_t id) : Device(id), m_device(device)
{
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
    tuh_configure(TUH_OPT_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &host_config);
    tuh_init(TUH_OPT_RHPORT);
}

void USBHostHardwareDevice::update(bool full_poll, bool send_events)
{
}


// void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report, uint16_t len)
// {
// printf("report! %d\r\n", len);
// if (report[0] == 0x81 && report[1] == 0x01)
// {
//     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
//     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
//     send_timeout = false;
// }
// else if (!send_timeout && report[0] == 0x81 && report[1] == 0x02)
// {
//     send_timeout = true;
//     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x03 /* PROCON_USB_ENABLE */};
//     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
// }
// else if (report[0] == 0x81 && report[1] == 0x03)
// {
//     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
//     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
// }
// else if (report[0] == 0x81 && report[1] == 0x02)
// {
//     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x04 /* PROCON_USB_ENABLE */};
//     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
// }
// if (report[0] == 0x30)
// {
//     for (int i = 0; i < len; i++)
//     {
//         printf("%02x, ", report[i]);
//     }
//     printf("\r\n");
// }
// tuh_hid_receive_report(dev_addr, idx);
// }
// void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report_desc, uint16_t desc_len)
// {

//     printf("mount! %d\r\n", desc_len);
//     tuh_hid_receive_report(dev_addr, idx);
// }
bool USBHostHardwareDevice::using_pin(uint8_t pin)
{
    return pin == m_device.firstPin || pin == m_device.firstPin + 1;
}
static std::unordered_map<uint8_t, std::shared_ptr<UsbHostDevice>> host_devices;
bool usbh_init(void)
{
    printf("usbh init\r\n");
    host_devices.clear();
    return true;
}

std::shared_ptr<UsbHostInterface> (*host_device_types[])(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len) = {
    XInputGamepadHost::open,
    XInputAudioHost::open,
    XInputModuleHost::open,
    XInputSecurityHost::open,
    XInputBigButtonHost::open};

bool usbh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    printf("usbh open %d\r\n", dev_addr);
    if (host_devices.find(dev_addr) == host_devices.end()) {
        host_devices[dev_addr] = std::make_unique<UsbHostDevice>(dev_addr, USBHostHardwareDevice::instance->m_id);
    }
    auto& list = host_devices[dev_addr];
    for (auto &host_device : host_device_types)
    {
        auto dev = host_device(list, desc_itf, max_len);
        if (dev != nullptr)
        {
            list->host_devices_by_itf[desc_itf->bInterfaceNumber] = dev;
            list->interfaces.push_back(dev);
            return true;
        }
    }
    return false;
}

bool usbh_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    return host_devices[dev_addr]->host_devices_by_itf[itf_num]->set_config();
}

bool usbh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    return host_devices[dev_addr]->host_devices_by_endpoint[ep_addr]->xfer_cb(ep_addr, result, xferred_bytes);
}

void usbh_close(uint8_t dev_addr)
{
    host_devices.erase(dev_addr);
}

usbh_class_driver_t driver_host[] = {
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