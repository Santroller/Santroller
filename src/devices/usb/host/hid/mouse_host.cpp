#include "tusb_option.h"
#include "devices/usb/host/hid/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"


MouseHost::MouseHost(uint8_t dev_addr, uint8_t interface, uint16_t id, HID_ReportInfo_t *info) : HidHost(dev_addr, interface, id), m_info(info)
{
    m_subtype = KeyboardMouse;
    m_decoder.set_report_info(info);
}

MouseHost::~MouseHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
    }
}

void MouseHost::disconnect()
{
    m_decoder.set_report_info(nullptr);
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
    HidHost::disconnect();
}

std::shared_ptr<UsbHostInterface> MouseHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();
    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceProtocol == HID_ITF_PROTOCOL_MOUSE)
    {
        auto intf = std::make_shared<MouseHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, info);
        uint8_t endpoints = itf_desc->bNumEndpoints;
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
        usb_host_add_assignable_interface(intf);
        return intf;
    }
    return nullptr;
}

bool MouseHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool MouseHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (result == XFER_RESULT_SUCCESS && xferred_bytes)
        {
            m_decoder.handle_report(m_ep_in_buf, time_us_32());
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool MouseHost::mouse_button(MouseButtonType button)
{
    switch (button)
    {
    case Mouse_Left:
        return m_decoder.button(HidMouseDecoder::Left);
    case Mouse_Right:
        return m_decoder.button(HidMouseDecoder::Right);
    case Mouse_Middle:
        return m_decoder.button(HidMouseDecoder::Middle);
    }
    return false;
}

uint16_t MouseHost::mouse_axis(MouseAxisType axis)
{
    return m_decoder.axis((HidMouseDecoder::Axis)(axis - Mouse_MoveX), time_us_32());
}

// Inputs picked in the config tool as a USB button / axis carry a mouse output as the type
bool MouseHost::tick_digital(proto_Output& type)
{
    if (type.which_mapping == proto_Output_mouseButton_tag)
    {
        return mouse_button(type.mapping.mouseButton);
    }
    if (type.which_mapping == proto_Output_mouseAxis_tag)
    {
        return mouse_axis(type.mapping.mouseAxis) != UINT16_MAX / 2;
    }
    return false;
}
uint16_t MouseHost::tick_analog(proto_Output& type)
{
    if (type.which_mapping == proto_Output_mouseAxis_tag)
    {
        return mouse_axis(type.mapping.mouseAxis);
    }
    return tick_digital(type) ? UINT16_MAX : 0;
}