#include "tusb_option.h"
#include "devices/usb/host/hid/ps4_host.h"
#include "usb/auth_broker.h"
#include "class/hid/hid.h"
#include "devices/usb.hpp"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

Ps4Host::~Ps4Host()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModePs4);
        auth_broker.unregister_auth_device(ModePs4);
        m_auth_registered = false;
    }
}

void Ps4Host::disconnect()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModePs4);
        auth_broker.unregister_auth_device(ModePs4);
        m_auth_registered = false;
    }

    UsbHostInterface::disconnect();
}

std::shared_ptr<UsbHostInterface> Ps4Host::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool isThirdParty = info->foundPS4Usage;
    bool isFirstParty = vid == SONY_VID && (pid == PS4_DS_PID_1 || pid == PS4_DS_PID_2 || pid == PS4_DS_PID_3);
    uint8_t data[48];
    tusb_control_request_t setup_input_caps = {
        bmRequestType_bit : {
            recipient : TUSB_REQ_RCPT_INTERFACE,
            type : TUSB_REQ_TYPE_CLASS,
            direction : TUSB_DIR_IN
        },
        bRequest : HID_REQ_CONTROL_GET_REPORT,
        wValue : 0x0303,
        wIndex : itf_desc->bInterfaceNumber,
        wLength : sizeof(data)
    };
    if (isFirstParty || isThirdParty)
    {
        auto intf = std::make_shared<Ps4Host>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
        intf->m_third_party = isThirdParty;
        if (isThirdParty)
        {
            // request capabilities for 3rd party gamepad
            intf->send_ctrl_xfer(setup_input_caps, data, nullptr);
            if (data[2] == 0x27)
            {
                uint8_t capabilities = data[4];
                uint8_t device_type = data[5];
                switch (device_type)
                {
                case 0x00:
                    intf->m_subtype = Gamepad;
                    break;
                case 0x01:
                    intf->m_subtype = RockBandGuitar;
                    break;
                case 0x02:
                    intf->m_subtype = RockBandDrums;
                    break;
                case 0x04:
                    intf->m_subtype = Dancepad;
                    break;
                case 0x06:
                    intf->m_subtype = Wheel;
                    break;
                case 0x07:
                    intf->m_subtype = FightStick;
                    break;
                case 0x08:
                    intf->m_subtype = FlightStick;
                    break;
                default:
                    intf->m_subtype = Gamepad;
                    break;
                }
                if (capabilities & 0x02)
                {
                    intf->m_sensors_supported = true;
                }
                if (capabilities & 0x04)
                {
                    intf->m_lightbar_supported = true;
                }
                if (capabilities & 0x08)
                {
                    intf->m_vibration_supported = true;
                }
                if (capabilities & 0x40)
                {
                    intf->m_touchpad_supported = true;
                }
            }
        }
        else
        {
            // everything supported on first party controller
            intf->m_sensors_supported = true;
            intf->m_lightbar_supported = true;
            intf->m_vibration_supported = true;
            intf->m_touchpad_supported = true;
        }
        p_desc = tu_desc_next(p_desc);
        tusb_hid_descriptor_hid_t *x_desc =
            (tusb_hid_descriptor_hid_t *)p_desc;
        TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
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
        printf("ps4 auth found\r\n");
        
        // Register as auth provider
        if (!auth_broker.has_handler(ModePs4))
        {
            auth_broker.register_handler(ModePs4, [intf](XGIPProtocol* packet) {
                // PS4 doesn't use XGIP, this is just for interface compatibility
            });
            // Also register the device itself for HID feature report auth
            auth_broker.register_auth_device(ModePs4, intf);
            intf->m_auth_registered = true;
        }
        DeviceManager::instance().add_assignable_usb_device(intf);
        USB_FreeReportInfo(info);
        return intf;
    }
    return nullptr;
}

bool Ps4Host::set_config()
{
    UsbHostInterface::set_config();
    return true;
}

bool Ps4Host::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool Ps4Host::tick_digital(proto_Output &type)
{
    PS4Dpad_Data_t *report = (PS4Dpad_Data_t *)m_ep_in_buf;
    uint8_t dpad = report->dpad >= 0x08 ? 0 : dpad_bindings_reverse[report->dpad];
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (PS4Gamepad_Data_t *)m_ep_in_buf;
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
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbButton_tag)
        {
            auto data = (PS4RockBandGuitar_Data_t *)m_ep_in_buf;
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
            auto data = (PS4GHLGuitar_Data_t *)m_ep_in_buf;
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
uint16_t Ps4Host::tick_analog(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (PS4Gamepad_Data_t *)m_ep_in_buf;
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
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (PS4GHLGuitar_Data_t *)m_ep_in_buf;
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
            auto data = (PS4RockBandGuitar_Data_t *)m_ep_in_buf;
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