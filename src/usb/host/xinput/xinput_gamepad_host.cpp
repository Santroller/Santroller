#include "tusb_option.h"
#include "usb/host/xinput_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "usb/usb_devices.h"
#include "config.hpp"

std::shared_ptr<UsbHostInterface> XInputGamepadHost::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *desc_itf, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass, nullptr);
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)desc_itf;

    if (desc_itf->bInterfaceSubClass == 0x5D && desc_itf->bInterfaceProtocol == 0x01)
    {
        auto intf = std::make_shared<XInputGamepadHost>(dev_addr, desc_itf->bInterfaceNumber, list->m_id);
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, nullptr);
        intf->m_subtype = get_subtype_from_xinput(x_desc->subtype);
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--)
        {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
            if (desc_ep->bEndpointAddress & 0x80)
            {
                intf->m_ep_in = desc_ep->bEndpointAddress;
                intf->m_ep_in_size = desc_ep->wMaxPacketSize;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep), nullptr);
                usbh_edpt_xfer(dev_addr, intf->m_ep_in, intf->m_ep_in_buf, intf->m_ep_in_size);
            }
            else
            {
                intf->m_ep_out = desc_ep->bEndpointAddress;
                intf->m_ep_out_size = desc_ep->wMaxPacketSize;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep), nullptr);
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
        printf("found device: %d\r\n", intf->m_subtype);
        return intf;
    }
    return nullptr;
}

bool XInputGamepadHost::set_config()
{
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
    if (ep_addr & 0x80)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool XInputGamepadHost::tick_digital(UsbButtonType type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonRed:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonYellow:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonBlue:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonOrange:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonBack:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonStrumUp:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->dpadUp;
        case UsbButtonStrumDown:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    case RockBandGuitar:
        switch (type)
        {
        case UsbButtonGreen:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonRed:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonYellow:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonBlue:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonOrange:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonBack:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonStrumUp:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->dpadUp;
        case UsbButtonStrumDown:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    case LiveGuitar:
        switch (type)
        {
        case UsbButtonBlack1:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->a;
        case UsbButtonBlack2:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->b;
        case UsbButtonBlack3:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->y;
        case UsbButtonWhite1:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->x;
        case UsbButtonWhite2:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonWhite3:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->start;
        case UsbButtonGuide:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonStrumUp:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->strumBar == 0x00;
        case UsbButtonStrumDown:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->strumBar == 0xFF;
        case UsbButtonDpadUp:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadDown:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        case UsbButtonDpadLeft:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    default:
        switch (type)
        {
        case UsbButtonA:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->a;
        case UsbButtonB:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->b;
        case UsbButtonX:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->x;
        case UsbButtonY:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->y;
        case UsbButtonLeftShoulder:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->leftShoulder;
        case UsbButtonRightShoulder:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->rightShoulder;
        case UsbButtonBack:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->back;
        case UsbButtonStart:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->start;
        case UsbButtonLeftThumbClick:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->leftThumbClick;
        case UsbButtonRightThumbClick:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->rightThumbClick;
        case UsbButtonGuide:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->guide;
        case UsbButtonDpadUp:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->dpadUp;
        case UsbButtonDpadDown:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->dpadDown;
        case UsbButtonDpadLeft:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->dpadLeft;
        case UsbButtonDpadRight:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->dpadRight;
        default:
            return false;
        }
        return false;
    }

    return false;
}
uint16_t XInputGamepadHost::tick_analog(UsbAxisType type)
{
    switch (m_subtype)
    {
    case GuitarHeroGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->whammy + INT16_MAX;
        case UsbAxisTilt:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->tilt + INT16_MAX;
        case UsbAxisSlider:
            return ((XInputGuitarHeroGuitar_Data_t *)m_ep_in_buf)->slider + INT16_MAX;
        }
        break;
    case LiveGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->whammy + INT16_MAX;
        case UsbAxisTilt:
            return ((XInputGHLGuitar_Data_t *)m_ep_in_buf)->tilt + INT16_MAX;
        }
        break;
    case RockBandGuitar:
        switch (type)
        {
        case UsbAxisWhammy:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->whammy + INT16_MAX;
        case UsbAxisTilt:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->tilt + INT16_MAX;
        case UsbAxisPickup:
            return ((XInputRockBandGuitar_Data_t *)m_ep_in_buf)->pickup + INT16_MAX;
        }
    default:
        switch (type)
        {
        case UsbAxisLeftTrigger:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->leftTrigger << 8;
        case UsbAxisRightTrigger:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->rightTrigger << 8;
        case UsbAxisLeftStickX:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->leftStickX + INT16_MAX;
        case UsbAxisLeftStickY:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->leftStickY + INT16_MAX;
        case UsbAxisRightStickX:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->rightStickX + INT16_MAX;
        case UsbAxisRightStickY:
            return ((XInputGamepad_Data_t *)m_ep_in_buf)->rightStickY + INT16_MAX;
        }
    }

    return 0;
}