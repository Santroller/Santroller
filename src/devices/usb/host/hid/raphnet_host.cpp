#include "tusb_option.h"
#include "devices/usb/host/hid/raphnet_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "utils.h"
#include "protocols/raphnet.hpp"

std::shared_ptr<UsbHostInterface> RaphnetHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    if (vid != RAPHNET_VID)
    {
        return nullptr;
    }
    if (itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE)
    {
        return nullptr;
    }
    uint8_t dev_addr = list->dev_addr();
    auto intf = std::make_shared<RaphnetHost>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);

    uint8_t data[] = {0x00, 0x00, 0x00};
    uint8_t data2[] = {RQ_RNT_GET_CONTROLLER_TYPE, 0x00, 0x00};
    for (int i = 0; i < 10; i++)
    {
        intf->set_report(0x00, HID_REPORT_TYPE_FEATURE, data2, sizeof(data2));
        intf->get_report(0x00, HID_REPORT_TYPE_FEATURE, data, sizeof(data));
        if (data[0])
        {
            break;
        }
        sleep_ms(100);
    }
    SubType subtype = Gamepad;
    switch (data[2])
    {
    case RNT_TYPE_WII_GUITAR:
        subtype = GuitarHeroGuitar;
        break;
    case RNT_TYPE_WII_DRUM:
        subtype = GuitarHeroDrums;
        break;
    case RNT_TYPE_WII_TURNTABLE:
        subtype = DjHeroTurntable;
        break;
    default:
        subtype = Gamepad;
        break;
    }
    intf->set_subtype(subtype);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
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
    usb_host_add_assignable_interface(intf);
    USB_FreeReportInfo(info);
    return intf;
}

bool RaphnetHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80 && result != XFER_RESULT_FAILED)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool RaphnetHost::set_config()
{
    UsbHostInterface::set_config();
    return true;
}

bool RaphnetHost::tick_digital(proto_Output &type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
    {
        auto data = (RaphnetGuitar_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            switch (type.mapping.ghButton)
            {
            case GuitarHeroGuitar_Green:
                return data->green;
            case GuitarHeroGuitar_Red:
                return data->red;
            case GuitarHeroGuitar_Yellow:
                return data->yellow;
            case GuitarHeroGuitar_Blue:
                return data->blue;
            case GuitarHeroGuitar_Orange:
                return data->orange;
            default:
                return false;
            }
        }
        if (type.which_mapping == proto_Output_rbButton_tag)
        {
            switch (type.mapping.rbButton)
            {
            case RockBandGuitar_Green:
                return data->green;
            case RockBandGuitar_Red:
                return data->red;
            case RockBandGuitar_Yellow:
                return data->yellow;
            case RockBandGuitar_Blue:
                return data->blue;
            case RockBandGuitar_Orange:
                return data->orange;
            default:
                return false;
            }
        }
        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:
                return data->green;
            case Gamepad_B:
                return data->red;
            case Gamepad_X:
                return data->blue;
            case Gamepad_Y:
                return data->yellow;
            case Gamepad_LeftShoulder:
                return data->orange;
            case Gamepad_Start:
                return data->plus;
            case Gamepad_Back:
                return data->minus;
            case Gamepad_DpadUp:
                return data->up;
            case Gamepad_DpadDown:
                return data->down;
            default:
                return false;
            }
        }
        return false;
    }
    case GuitarHeroDrums:
    {
        auto data = (RaphnetDrum_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:
                return data->green;
            case Gamepad_B:
                return data->red;
            case Gamepad_X:
                return data->blue;
            case Gamepad_Y:
                return data->yellow;
            case Gamepad_LeftShoulder:
                return data->orange;
            case Gamepad_Start:
                return data->plus;
            case Gamepad_Back:
                return data->minus;
            default:
                return false;
            }
        }
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            switch (type.mapping.ghButton)
            {
            case GuitarHeroGuitar_Green:
                return data->green;
            case GuitarHeroGuitar_Red:
                return data->red;
            case GuitarHeroGuitar_Yellow:
                return data->yellow;
            case GuitarHeroGuitar_Blue:
                return data->blue;
            case GuitarHeroGuitar_Orange:
                return data->orange;
            default:
                return false;
            }
        }
        return false;
    }
    case Gamepad:
    default:
    {
        auto data = (RaphnetGamepad_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
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
            case Gamepad_Start:
                return data->start;
            case Gamepad_Back:
                return data->select;
            case Gamepad_Guide:
                return data->home;
            case Gamepad_DpadUp:
                return data->up;
            case Gamepad_DpadDown:
                return data->down;
            case Gamepad_DpadLeft:
                return data->left;
            case Gamepad_DpadRight:
                return data->right;
            default:
                return false;
            }
        }
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            switch (type.mapping.ghButton)
            {
            case GuitarHeroGuitar_Green:
                return data->a;
            case GuitarHeroGuitar_Red:
                return data->b;
            case GuitarHeroGuitar_Yellow:
                return data->y;
            case GuitarHeroGuitar_Blue:
                return data->x;
            case GuitarHeroGuitar_Orange:
                return data->leftShoulder;
            default:
                return false;
            }
        }
        return false;
    }
    case DjHeroTurntable:
    {
        auto data = (RaphnetTurntable_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_djhButton_tag)
        {
            switch (type.mapping.djhButton)
            {
            case DJHTurntable_LeftGreen:
                return data->leftGreen;
            case DJHTurntable_LeftRed:
                return data->leftRed;
            case DJHTurntable_LeftBlue:
                return data->leftBlue;
            case DJHTurntable_RightGreen:
                return data->rightGreen;
            case DJHTurntable_RightRed:
                return data->rightRed;
            case DJHTurntable_RightBlue:
                return data->rightBlue;
            default:
                return false;
            }
        }
        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:
                return data->leftGreen || data->rightGreen;
            case Gamepad_B:
                return data->leftRed || data->rightRed;
            case Gamepad_X:
                return data->leftBlue || data->rightBlue;
            case Gamepad_Y:
                return data->euphoria;
            case Gamepad_Start:
                return data->plus;
            case Gamepad_Back:
                return data->minus;
            default:
                return false;
            }
        }
        return false;
    }
    }
    return false;
}

uint16_t RaphnetHost::tick_analog(proto_Output &type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
    {
        auto data = (RaphnetGuitar_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:
                return map_16(data->joyX, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_LeftStickY:
                return map_16(data->joyY, 0, RAPHNET_MAX, 0, 65535);
            default:
                return 0;
            }
        }
        if (type.which_mapping == proto_Output_ghAxis_tag)
        {
            switch (type.mapping.ghAxis)
            {
            case GuitarHeroGuitar_Whammy:
                return data->whammy;
            default:
                return 0;
            }
        }
        if (type.which_mapping == proto_Output_rbAxis_tag)
        {
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return data->whammy;
            default:
                return 0;
            }
        }
        return 0;
    }
    case GuitarHeroDrums:
    {
        auto data = (RaphnetDrum_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:
                return map_16(data->joyX, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_LeftStickY:
                return map_16(data->joyY, 0, RAPHNET_MAX, 0, 65535);
            default:
                return 0;
            }
        }
        if (type.which_mapping == proto_Output_ghDrumAxis_tag)
        {
            switch (type.mapping.ghDrumAxis)
            {
            case GuitarHeroDrums_RedPad:
                return data->red ? UINT16_MAX : 0;
            case GuitarHeroDrums_YellowPad:
                return data->yellow ? UINT16_MAX : 0;
            case GuitarHeroDrums_BluePad:
                return data->blue ? UINT16_MAX : 0;
            case GuitarHeroDrums_OrangePad:
                return data->orange ? UINT16_MAX : 0;
            case GuitarHeroDrums_GreenPad:
                return data->green ? UINT16_MAX : 0;
            default:
                return 0;
            }
        }
        if (type.which_mapping == proto_Output_rbDrumAxis_tag)
        {
            switch (type.mapping.rbDrumAxis)
            {
            case RockBandDrums_RedPad:
                return data->red ? UINT16_MAX : 0;
            case RockBandDrums_YellowPad:
                return data->yellow ? UINT16_MAX : 0;
            case RockBandDrums_BluePad:
                return data->blue ? UINT16_MAX : 0;
            case RockBandDrums_GreenPad:
                return data->green ? UINT16_MAX : 0;
            default:
                return 0;
            }
        }
        return 0;
    }
    case DjHeroTurntable:
    {
        auto data = (RaphnetTurntable_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:
                return map_16(data->joyX, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_LeftStickY:
                return map_16(data->joyY, 0, RAPHNET_MAX, 0, 65535);
            default:
                return 0;
            }
        }
        if (type.which_mapping == proto_Output_djhAxis_tag)
        {
            switch (type.mapping.djhAxis)
            {
            case DJHTurntable_LeftVelocity:
                return map_16(data->leftTurntable, 0, RAPHNET_MAX, 0, 65535);
            case DJHTurntable_RightVelocity:
                return map_16(data->rightTurntable, 0, RAPHNET_MAX, 0, 65535);
            case DJHTurntable_Crossfader:
                return map_16(data->crossfader, 0, RAPHNET_MAX, 0, 65535);
            case DJHTurntable_EffectsKnob:
                return map_16(data->effectDial, 0, RAPHNET_MAX, 0, 65535);
            default:
                return 0;
            }
        }
        return 0;
    }
    case Gamepad:
    default:
    {
        auto data = (RaphnetGamepad_Data_t *)m_ep_in_buf;
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:
                return map_16(data->leftJoyX, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_LeftStickY:
                return map_16(data->leftJoyY, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_RightStickX:
                return map_16(data->rightJoyX, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_RightStickY:
                return map_16(data->rightJoyY, 0, RAPHNET_MAX, 0, 65535);
            case Gamepad_LeftTrigger:
                return data->leftTrigger;
            case Gamepad_RightTrigger:
                return data->rightTrigger;
            default:
                return 0;
            }
        }
        return 0;
    }
    }
    return 0;
}