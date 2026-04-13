#include "tusb_option.h"
#include "usb/host/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"
#include "hidparser.h"
#include "protocols/dance_pad.hpp"

std::shared_ptr<UsbHostInterface> StepmaniaHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE || vid != ARDUINO_VID || pid != STEPMANIA_X_PID)
    {
        return nullptr;
    }
    auto intf = std::make_shared<StepmaniaHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
    uint8_t endpoints = itf_desc->bNumEndpoints;
    p_desc = tu_desc_next(p_desc);
    tusb_hid_descriptor_hid_t *x_desc =
        (tusb_hid_descriptor_hid_t *)p_desc;
    TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
    while (endpoints--)
    {
        p_desc = tu_desc_next(p_desc);
        tusb_desc_endpoint_t const *desc_ep =
            (tusb_desc_endpoint_t const *)p_desc;
        TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
        if (desc_ep->bEndpointAddress & 0x80)
        {
            intf->m_ep_in = desc_ep->bEndpointAddress;
            intf->m_ep_in_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
            usbh_edpt_xfer(dev_addr, intf->m_ep_in, intf->m_ep_in_buf, intf->m_ep_in_size);
        }
        else
        {
            intf->m_ep_out = desc_ep->bEndpointAddress;
            intf->m_ep_out_size = desc_ep->wMaxPacketSize;
            TU_VERIFY(tuh_edpt_open(dev_addr, desc_ep), nullptr);
        }
    }
    if (intf->m_ep_out)
    {
        list->host_devices_by_endpoint_out[intf->m_ep_out] = intf;
    }
    if (intf->m_ep_in)
    {
        list->host_devices_by_endpoint_in[intf->m_ep_in & (~0x80)] = intf;
    }
    assignable_usb_devices.push_back(intf);
    USB_FreeReportInfo(info);
    return intf;
}

bool StepmaniaHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (m_ep_in_buf[0] == STEPMANIA_X_REPORT_ID) {
            memcpy(&m_last_input_report, m_ep_in_buf, m_ep_in_size);
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool StepmaniaHost::set_config()
{
    UsbHostInterface::set_config();
    return true;
}
bool StepmaniaHost::tick_digital(UsbButtonType type)
{
    switch (type)
    {
    case UsbButtonDpadUp:
        return m_last_input_report.dpadUp;
    case UsbButtonDpadDown:
        return m_last_input_report.dpadDown;
    case UsbButtonDpadLeft:
        return m_last_input_report.dpadLeft;
    case UsbButtonDpadRight:
        return m_last_input_report.dpadRight;
    default:
        return false;
    }
}
uint16_t StepmaniaHost::tick_analog(UsbAxisType type)
{
    return 0;
}