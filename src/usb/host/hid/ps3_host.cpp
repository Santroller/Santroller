#include "tusb_option.h"
#include "usb/host/hid_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"
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
        assignable_usb_devices.push_back(intf);
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

bool Ps3Host::tick_digital(UsbButtonType type)
{
    if (!m_third_party)
    {
        switch (type)
        {
        case UsbButtonA:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->a;
        case UsbButtonB:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->b;
        case UsbButtonX:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->x;
        case UsbButtonY:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->y;
        case UsbButtonLeftShoulder:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonRightShoulder:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->start;
        case UsbButtonLeftThumbClick:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->leftThumbClick;
        case UsbButtonRightThumbClick:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->rightThumbClick;
        case UsbButtonGuide:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonDpadUp:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->dpadUp;
        case UsbButtonDpadDown:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
    }
    PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)m_ep_in_buf;
    uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
    asm volatile("" ::
                     : "memory");
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonRed:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonYellow:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonBlue:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonOrange:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonBack:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonDpadUp:
        case UsbButtonStrumUp:
            return up;
        case UsbButtonDpadDown:
        case UsbButtonStrumDown:
            return down;
        case UsbButtonDpadLeft:
            return left;
        case UsbButtonDpadRight:
            return right;
        default:
            return false;
        }
        return false;
    case RockBandGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonRed:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonYellow:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonBlue:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonOrange:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonBack:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonDpadUp:
        case UsbButtonStrumUp:
            return up;
        case UsbButtonDpadDown:
        case UsbButtonStrumDown:
            return down;
        case UsbButtonDpadLeft:
            return left;
        case UsbButtonDpadRight:
            return right;
        default:
            return false;
        }
        return false;
    case LiveGuitar:
        switch (type)
        {
        case UsbButtonBlack1:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonBlack2:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonBlack3:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonWhite1:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonWhite2:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonWhite3:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonStrumUp:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->strumBar == 0x00;
        case UsbButtonStrumDown:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->strumBar == 0xFF;
        case UsbButtonDpadUp:
            return up;
        case UsbButtonDpadDown:
            return down;
        case UsbButtonDpadLeft:
            return left;
        case UsbButtonDpadRight:
            return right;
        default:
            return false;
        }
        return false;
    default:
        switch (type)
        {
        case UsbButtonA:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->a;
        case UsbButtonB:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->b;
        case UsbButtonX:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->x;
        case UsbButtonY:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->y;
        case UsbButtonLeftShoulder:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonRightShoulder:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->start;
        case UsbButtonLeftThumbClick:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->leftThumbClick;
        case UsbButtonRightThumbClick:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->rightThumbClick;
        case UsbButtonGuide:
            return ((PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonDpadUp:
            return up;
        case UsbButtonDpadDown:
            return down;
        case UsbButtonDpadLeft:
            return left;
        case UsbButtonDpadRight:
            return right;
        default:
            return false;
        }
        return false;
    }

    return false;
}
uint16_t Ps3Host::tick_analog(UsbAxisType type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->whammy << 8;
        case UsbAxisTilt:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->tilt << 2;
        case UsbAxisSlider:
            return ((PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf)->slider << 8;
        default:
            return 0;
        }
        break;
    case LiveGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->whammy << 8;
        case UsbAxisTilt:
            return ((PS3GHLGuitar_Data_t *)m_ep_in_buf)->tilt << 2;
        default:
            return 0;
        }
        break;
    case RockBandGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->whammy << 8;
        case UsbAxisTilt:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->tilt << 8;
        case UsbAxisPickup:
            return ((PS3RockBandGuitar_Data_t *)m_ep_in_buf)->pickup << 8;
        default:
            return 0;
        }
    default:
        switch (type)
        {
        case UsbAxisLeftTrigger:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->leftTrigger << 8;
        case UsbAxisRightTrigger:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->rightTrigger << 8;
        case UsbAxisLeftStickX:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->leftStickX << 8;
        case UsbAxisLeftStickY:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->leftStickY << 8;
        case UsbAxisRightStickX:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->rightStickX << 8;
        case UsbAxisRightStickY:
            return ((PS3Gamepad_Data_t *)m_ep_in_buf)->rightStickY << 8;
        default:
            return 0;
        }
    }

    return 0;
}