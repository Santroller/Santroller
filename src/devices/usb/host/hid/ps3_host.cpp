#include "tusb_option.h"
#include "devices/usb/host/hid/ps3_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

std::shared_ptr<UsbHostInterface> Ps3Host::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool isThirdParty = info->foundPS3Usage;
    bool isValid = isThirdParty;
    bool rb2 = false;
    bool ion = false;
    SubType subtype = Gamepad;
    switch (vid)
    {
    case SONY_VID:
        switch (pid)
        {
        case SONY_DS3_PID:
            isValid = true;
            break;
        }
        break;
    case REDOCTANE_VID:
        switch (pid)
        {
        case PS3_GH_GUITAR_PID:
            subtype = GuitarHeroGuitar;
            break;
        case PS3_GH_DRUM_PID:
            subtype = GuitarHeroDrums;
            break;
        case PS3_RB_GUITAR_PID:
            subtype = RockBandGuitar;
            break;
        case PS3_MPA_DRUM_PID:
            rb2 = true;
            subtype = RockBandDrums;
            break;
        case PS3_RB_DRUM_PID:
            rb2 = revision != 0x1000;
            subtype = RockBandDrums;
            break;
        case PS3_DJ_TURNTABLE_PID:
            subtype = DjHeroTurntable;
            break;
        case PS3WIIU_GHLIVE_DONGLE_PID:
            subtype = LiveGuitar;
            break;
        case PS3_MPA_KEYBOARD_PID:
        case PS3_KEYBOARD_PID:
            subtype = ProKeys;
            break;
        case PS3_MUSTANG_PID:
        case PS3_MUSTANG_MPA_PID:
            subtype = ProGuitarMustang;
            break;
        case PS3_SQUIRE_PID:
        case PS3_SQUIRE_MPA_PID:
            subtype = ProGuitarSquire;
            break;
        }
        break;

    case HARMONIX_VID:
        // Polled the same as PS3, so treat them as PS3 instruments
        switch (pid)
        {
        case WII_RB_GUITAR_PID:
        case WII_RB_GUITAR_2_PID:
            subtype = RockBandGuitar;
            break;

        case WII_RB_DRUM_PID:
            rb2 = false;
            subtype = RockBandDrums;
            break;
        case WII_RB_DRUM_2_PID:
        case WII_MPA_DRUMS_PID:
            rb2 = true;
            subtype = RockBandDrums;
            break;
        case WII_KEYBOARD_PID:
        case WII_MPA_KEYBOARD_PID:
            subtype = ProKeys;
            break;
        case WII_MUSTANG_PID:
        case WII_MUSTANG_MPA_PID:
            subtype = ProGuitarMustang;
            break;
        case WII_SQUIRE_PID:
        case WII_SQUIRE_MPA_PID:
            subtype = ProGuitarSquire;
            break;
        case XBOX_360_ION_ROCKER_VID:
            rb2 = true;
            ion = true;
            subtype = RockBandDrums;
            break;
        }

        break;
    }
    if (isValid)
    {
        auto intf = std::make_shared<Ps3Host>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, isThirdParty, rb2, ion, subtype);
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
        DeviceManager::instance().add_assignable_usb_device(intf);
        USB_FreeReportInfo(info);
        return intf;
    }
    return nullptr;
}

bool Ps3Host::set_config()
{
    UsbHostInterface::set_config();
    return true;
}

bool Ps3Host::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool Ps3Host::tick_digital(proto_Output &type)
{
    if (!m_third_party)
    {
        // first party was only ever gamepads
        if (type.which_mapping == proto_Output_gamepadButton_tag)
        {
            auto data = (PS3Gamepad_Data_t *)m_ep_in_buf;
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
        return false;
    }
    PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)m_ep_in_buf;
    uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
    asm volatile("" ::
                     : "memory");
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf;
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
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            auto data = (PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf;
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
            auto data = (PS3RockBandGuitar_Data_t *)m_ep_in_buf;
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
            auto data = (PS3GHLGuitar_Data_t *)m_ep_in_buf;
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
uint16_t Ps3Host::tick_analog(proto_Output &type)
{
    if (!m_third_party)
    {
        // first party was only ever gamepads
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            auto data = (PS3Gamepad_Data_t *)m_ep_in_buf;
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftTrigger:
                return data->leftTrigger << 8;
            case Gamepad_RightTrigger:
                return data->rightTrigger << 8;
            case Gamepad_LeftStickX:
                return data->leftStickX << 8;
            case Gamepad_LeftStickY:
                return data->leftStickY << 8;
            case Gamepad_RightStickX:
                return data->rightStickX << 8;
            case Gamepad_RightStickY:
                return data->rightStickY << 8;
            default:
                return 0;
            }
        }
        return 0;
    }
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (PS3Dpad_Data_t *)m_ep_in_buf;
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftTrigger:
            return data->leftTrigger << 8;
        case Gamepad_RightTrigger:
            return data->rightTrigger << 8;
        case Gamepad_LeftStickX:
            return data->leftStickX << 8;
        case Gamepad_LeftStickY:
            return data->leftStickY << 8;
        case Gamepad_RightStickX:
            return data->rightStickX << 8;
        case Gamepad_RightStickY:
            return data->rightStickY << 8;
        default:
            return 0;
        }
    }
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghAxis_tag)
        {
            auto data = (PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.ghAxis)
            {
            case GuitarHeroGuitar_Whammy:
                return data->whammy << 8;
            case GuitarHeroGuitar_Tilt:
                return data->tilt << 2;
            default:
                return 0;
            }
        }
        break;
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (PS3GHLGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.ghlAxis)
            {
            case GuitarHeroLiveGuitar_Whammy:
                return data->whammy << 8;
            case GuitarHeroLiveGuitar_Tilt:
                return data->tilt << 2;
            default:
                return 0;
            }
        }
        break;
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbAxis_tag)
        {
            auto data = (PS3RockBandGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return data->whammy << 8;
            case RockBandGuitar_Tilt:
                return data->tilt << 8;
            case RockBandGuitar_Pickup:
                return data->tilt << 8;
            default:
                return 0;
            }
        }
    default:
        break;
    }

    return 0;
}