#include "tusb_option.h"
#include "devices/usb/host/hid/misc_hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

#define XINPUT_COMPAT_INPUT_REPORT_ID 1
#define XINPUT_COMPAT_GUIDE_REPORT_ID 2

// Xbox One / Series controllers that enumerate as a HID device instead of using GIP
static bool is_xinput_compat(uint16_t vid, uint16_t pid)
{
    return vid == XBOX_ONE_CONTROLLER_VID && ((pid >= 0x02e0 && pid <= 0x02ef) || pid == XBOX_SERIES_BT_PID);
}

std::shared_ptr<UsbHostInterface> XInputCompatHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE || !is_xinput_compat(vid, pid))
    {
        return nullptr;
    }
    auto intf = std::make_shared<XInputCompatHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
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
    USB_FreeReportInfo(info);
    return intf;
}

bool XInputCompatHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (m_ep_in_buf[0] == XINPUT_COMPAT_INPUT_REPORT_ID)
        {
            memcpy(&m_last_input_report, m_ep_in_buf, tu_min32(xferred_bytes, sizeof(m_last_input_report)));
        }
        else if (m_ep_in_buf[0] == XINPUT_COMPAT_GUIDE_REPORT_ID)
        {
            // Some firmware revisions report the guide button separately
            m_guide = ((XInputCompatGuide_Data_t *)m_ep_in_buf)->guide;
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputCompatHost::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}
bool XInputCompatHost::tick_digital(proto_Output &type)
{
    // dpad is a 1-based hat, 0 is neutral
    uint8_t hat = m_last_input_report.dpad;
    uint8_t dpad = (hat == 0 || hat > 8) ? 0 : dpad_bindings_reverse[hat - 1];
    asm volatile("" ::
                     : "memory");
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = &m_last_input_report;
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:
            return data->a;
        case Gamepad_B:
            return data->b;
        case Gamepad_X:
            return data->x;
        case Gamepad_Y:
            return data->y;
        case Gamepad_LeftShoulder:
            return data->leftShoulder;
        case Gamepad_RightShoulder:
            return data->rightShoulder;
        case Gamepad_Back:
            return data->back;
        case Gamepad_Start:
            return data->start;
        case Gamepad_LeftThumbClick:
            return data->leftThumbClick;
        case Gamepad_RightThumbClick:
            return data->rightThumbClick;
        case Gamepad_Guide:
            return data->guide || m_guide;
        case Gamepad_Capture:
            return data->capture;
        case Gamepad_DpadUp:
            return up;
        case Gamepad_DpadDown:
            return down;
        case Gamepad_DpadLeft:
            return left;
        case Gamepad_DpadRight:
            return right;
        default:
            return false;
        }
    }
    return false;
}
uint16_t XInputCompatHost::tick_analog(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = &m_last_input_report;
        switch (type.mapping.gamepadAxis)
        {
        // triggers are 10 bit
        case Gamepad_LeftTrigger:
            return data->leftTrigger << 6;
        case Gamepad_RightTrigger:
            return data->rightTrigger << 6;
        case Gamepad_LeftStickX:
            return data->leftStickX;
        case Gamepad_LeftStickY:
            return UINT16_MAX - data->leftStickY;
        case Gamepad_RightStickX:
            return data->rightStickX;
        case Gamepad_RightStickY:
            return UINT16_MAX - data->rightStickY;
        default:
            return 0;
        }
    }
    return 0;
}
