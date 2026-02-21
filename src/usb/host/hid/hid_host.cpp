#include "tusb_option.h"
#include "usb/host/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"
#include "hidparser.h"
#include "usb/host/hid_host.h"

static std::shared_ptr<UsbHostInterface> (*hid_device_types[])(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info) = {
    Ps3Host::open,
    Ps4Host::open,
    Ps5Host::open,
    KeyboardHost::open,
    MouseHost::open};
std::shared_ptr<UsbHostInterface> HidHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_HID == desc_itf->bInterfaceClass, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc =
        (tusb_hid_descriptor_hid_t *)p_desc;
    TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);

    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);
    static uint8_t temp_buf[512];
    tuh_descriptor_get_hid_report(dev_addr, desc_itf->bInterfaceNumber, x_desc->bReportType, 0, temp_buf, x_desc->wReportLength, NULL, (uintptr_t)temp_buf);
    // Seems that sometimes we miss the first byte?
    if (!temp_buf[0] || temp_buf[0] == 4)
    {
        temp_buf[0] = 0x05;
    }
    HID_ReportInfo_t* info;
    USB_ProcessHIDReport(temp_buf, x_desc->wReportLength, &info);
    tusb_desc_device_t desc;
    tuh_descriptor_get_device_sync(dev_addr, &desc, sizeof(tusb_desc_device_t));
    for (auto& open: hid_device_types) {
        auto ret = open(list, desc_itf, max_len, vid, pid, desc.bcdDevice, info);
        if (ret != nullptr) {
            return ret;
        }
    }

    return nullptr;
}
