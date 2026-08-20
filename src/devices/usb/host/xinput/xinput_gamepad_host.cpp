#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"

std::shared_ptr<UsbHostInterface> XInputGamepadHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len, uint16_t *out_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass && desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x01, nullptr);
    uint8_t dev_addr = list->dev_addr();
    uint16_t len = 0;
    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    auto intf = std::make_shared<XInputGamepadHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
    len += desc_itf->bLength;
    p_desc = tu_desc_next(p_desc);
    XBOX_ID_DESCRIPTOR *x_desc =
        (XBOX_ID_DESCRIPTOR *)p_desc;
    len += x_desc->bLength;
    TU_VERIFY(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, nullptr);
    intf->m_subtype = get_subtype_from_xinput(x_desc->subtype);
    uint8_t endpoints = desc_itf->bNumEndpoints;
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
        len += desc_ep->bLength;
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
    *out_len = len;
    return intf;
}

bool XInputGamepadHost::set_config()
{
    UsbHostInterface::set_config();
    XInputInputCapabilities_t caps;
    XInputVibrationCapabilities_t caps_vibr;
    uint32_t serial;

    tusb_control_request_t setup_input_caps = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : INPUT_CAPABILITIES_WVALUE,
        wIndex : m_interface,
        wLength : sizeof(XInputInputCapabilities_t)
    };
    tusb_control_request_t setup_vibration_caps = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : VIBRATION_CAPABILITIES_WVALUE,
        wIndex : m_interface,
        wLength : sizeof(XInputVibrationCapabilities_t)
    };
    tusb_control_request_t setup_serial = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_DEVICE,
            type : TUSB_REQ_TYPE_VENDOR,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : SERIAL_NUMBER_WVALUE,
        wIndex : m_interface,
        wLength : sizeof(serial)
    };
    // request serial, some controllers might expect this
    send_ctrl_xfer(setup_serial, &serial, nullptr);
    // request input capabilities, lets us differenciate controllers
    if (send_ctrl_xfer(setup_input_caps, &caps, nullptr) != 0)
    {
        // GHL guitars set no navigation
        if (m_subtype == GuitarHeroGuitar && caps.flags & XINPUT_FLAGS_NO_NAV)
        {
            m_subtype = LiveGuitar;
        }
        // GH drums don't set force feedback
        if (m_subtype == RockBandDrums && (caps.flags & XINPUT_FLAGS_FORCE_FEEDBACK) == 0)
        {
            m_subtype = GuitarHeroDrums;
        }
        // Pro guitars we have to identify by vid+pid
        if (caps.leftThumbX == HARMONIX_VID)
        {
            switch (caps.leftThumbY)
            {
            case XBOX_360_MUSTANG_PID:
            case XBOX_360_MPA_MUSTANG_PID:
                m_subtype = ProGuitarMustang;
                break;
            case XBOX_360_MPA_SQUIRE_PID:
            case XBOX_360_SQUIRE_PID:
                m_subtype = ProGuitarSquire;
                break;
            }
        }
    }
    // request vibration caps since some devices expect it
    send_ctrl_xfer(setup_vibration_caps, &caps_vibr, nullptr);
    return true;
}

bool XInputGamepadHost::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80 && result != XFER_RESULT_FAILED)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputGamepadHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (XInputGamepad_Data_t *)m_ep_in_buf;
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
            return data->guide;
        case Gamepad_DpadUp:
            return data->dpadUp;
        case Gamepad_DpadDown:
            return data->dpadDown;
        case Gamepad_DpadLeft:
            return data->dpadLeft;
        case Gamepad_DpadRight:
            return data->dpadRight;
        default:
            return false;
        }
    }
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            auto data = (XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf;
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
            case GuitarHeroGuitar_TapGreen:
                // TODO: this
                return false;
            case GuitarHeroGuitar_TapRed:
                // TODO: this
                return false;
            case GuitarHeroGuitar_TapYellow:
                // TODO: this
                return false;
            case GuitarHeroGuitar_TapBlue:
                // TODO: this
                return false;
            case GuitarHeroGuitar_TapOrange:
                // TODO: this
                return false;
            default:
                return false;
            }
        }
        return false;
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbButton_tag)
        {
            auto data = (XInputRockBandGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.rbButton)
            {
            case RockBandGuitar_Green:
                return data->a && !data->solo;
            case RockBandGuitar_Red:
                return data->b && !data->solo;
            case RockBandGuitar_Yellow:
                return data->y && !data->solo;
            case RockBandGuitar_Blue:
                return data->x && !data->solo;
            case RockBandGuitar_Orange:
                return data->leftShoulder && !data->solo;
            case RockBandGuitar_SoloGreen:
                return data->a && data->solo;
            case RockBandGuitar_SoloRed:
                return data->b && data->solo;
            case RockBandGuitar_SoloYellow:
                return data->y && data->solo;
            case RockBandGuitar_SoloBlue:
                return data->x && data->solo;
            case RockBandGuitar_SoloOrange:
                return data->leftShoulder && data->solo;
            default:
                return false;
            }
        }
        return false;
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlButton_tag)
        {
            auto data = (XInputGHLGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.ghlButton)
            {
            case GuitarHeroLiveGuitar_Black1:
                return data->a;
            case GuitarHeroLiveGuitar_Black2:
                return data->b;
            case GuitarHeroLiveGuitar_Black3:
                return data->y;
            case GuitarHeroLiveGuitar_White1:
                return data->x;
            case GuitarHeroLiveGuitar_White2:
                return data->leftShoulder;
            case GuitarHeroLiveGuitar_White3:
                return data->rightShoulder;
            case GuitarHeroLiveGuitar_StrumUp:
                return data->strumBar == 0x00;
            case GuitarHeroLiveGuitar_StrumDown:
                return data->strumBar == 0xFF;
            default:
                return false;
            }
        }
        return false;
    default:
        return false;
    }

    return false;
}
uint16_t XInputGamepadHost::tick_analog(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (XInputGamepad_Data_t *)m_ep_in_buf;
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftTrigger:
            return data->leftTrigger << 8;
        case Gamepad_RightTrigger:
            return data->rightTrigger << 8;
        case Gamepad_LeftStickX:
            return data->leftStickX + INT16_MAX;
        case Gamepad_LeftStickY:
            return data->leftStickY + INT16_MAX;
        case Gamepad_RightStickX:
            return data->rightStickX + INT16_MAX;
        case Gamepad_RightStickY:
            return data->rightStickY + INT16_MAX;
        default:
            return 0;
        }
    }
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghAxis_tag)
        {
            auto data = (XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.ghAxis)
            {
            case GuitarHeroGuitar_Whammy:
                return data->whammy + INT16_MAX;
            case GuitarHeroGuitar_Tilt:
                return data->tilt + INT16_MAX;
            default:
                return 0;
            }
        }
        break;
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (XInputGHLGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.ghlAxis)
            {
            case GuitarHeroLiveGuitar_Whammy:
                return data->whammy + INT16_MAX;
            case GuitarHeroLiveGuitar_Tilt:
                return data->tilt + INT16_MAX;
            default:
                return 0;
            }
        }
        break;
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbAxis_tag)
        {
            auto data = (XInputRockBandGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return data->whammy + INT16_MAX;
            case RockBandGuitar_Tilt:
                return data->tilt + INT16_MAX;
            case RockBandGuitar_Pickup:
                return data->tilt + INT16_MAX;
            default:
                return 0;
            }
        }
    default:
        break;
    }

    return 0;
}