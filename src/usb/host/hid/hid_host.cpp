#include "tusb_option.h"
#include "usb/host/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"
#include "hidparser.h"
#include "usb/host/hid_host.h"

const uint8_t HidHost::dpad_bindings_reverse[] = {UP, UP | RIGHT, RIGHT, DOWN | RIGHT, DOWN, DOWN | LEFT, LEFT, UP | LEFT};
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
    printf("hidhost_open: %02x\r\n", dev_addr);

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
    HID_ReportInfo_t *info;
    USB_ProcessHIDReport(temp_buf, x_desc->wReportLength, &info);
    tusb_desc_device_t desc;
    tuh_descriptor_get_device_sync(dev_addr, &desc, sizeof(tusb_desc_device_t));
    for (auto &open : hid_device_types)
    {
        auto ret = open(list, desc_itf, max_len, vid, pid, desc.bcdDevice, info);
        if (ret != nullptr)
        {
            return ret;
        }
    }
    USB_FreeReportInfo(info);
    return nullptr;
}

uint32_t HidHost::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen, bool *status)
{
    tusb_control_request_t get_report = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_CLASS,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : tu_u16(report_type, report_id),
        wIndex : m_interface,
        wLength : reqlen
    };
    return send_ctrl_xfer(get_report, buffer, status);
}
uint32_t HidHost::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t bufsize, bool *status)
{
    tusb_control_request_t set_report = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_CLASS,
            direction : TUSB_DIR_OUT
        },
        bRequest : HID_REQ_CONTROL_SET_REPORT,
        wValue : tu_u16(report_type, report_id),
        wIndex : m_interface,
        wLength : bufsize
    };
    return send_ctrl_xfer(set_report, buffer, status);
}