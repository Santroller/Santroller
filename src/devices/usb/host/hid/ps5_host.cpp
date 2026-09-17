#include "tusb_option.h"
#include "devices/usb/host/hid/ps5_host.h"
#include "usb/auth_broker.h"
#include "class/hid/hid.h"
#include "devices/usb.hpp"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"

Ps5Host::~Ps5Host()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModePs5);
        auth_broker.unregister_auth_device(ModePs5);
        m_auth_registered = false;
    }
}

void Ps5Host::disconnect()
{
    if (m_auth_registered)
    {
        auth_broker.unregister_handler(ModePs5);
        auth_broker.unregister_auth_device(ModePs5);
        m_auth_registered = false;
    }

    UsbHostInterface::disconnect();
}

bool ps5_parse_capabilities(const uint8_t *data, uint16_t len,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad)
{
    const uint8_t *caps = nullptr;
    if (len >= 6 && data[2] == 0x28)
    {
        caps = &data[2];
    }
    else if (len >= 4 && data[0] == 0x28)
    {
        caps = &data[0];
    }
    else if (len >= 5 && data[1] == 0x28)
    {
        caps = &data[1];
    }

    if (!caps)
        return false;

    uint8_t capabilities = caps[2];
    uint8_t device_type = caps[3];
    switch (device_type)
    {
    case 0x00:
        subtype = Gamepad;
        break;
    case 0x01:
        subtype = RockBandGuitar;
        break;
    case 0x02:
        subtype = RockBandDrums;
        break;
    case 0x04:
        subtype = Dancepad;
        break;
    case 0x06:
        subtype = Wheel;
        break;
    case 0x07:
        subtype = FightStick;
        break;
    case 0x08:
        subtype = FlightStick;
        break;
    default:
        subtype = Gamepad;
        break;
    }
    if (capabilities & 0x02)
    {
        sensors = true;
    }
    if (capabilities & 0x04)
    {
        lightbar = true;
    }
    if (capabilities & 0x08)
    {
        vibration = true;
    }
    if (capabilities & 0x40)
    {
        touchpad = true;
    }
    return true;
}

std::shared_ptr<UsbHostInterface> Ps5Host::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool isThirdParty = info ? info->foundPS5Usage : false;
    bool isFirstParty = vid == SONY_VID && (pid == PS5_DS_PID || pid == PS5_DS_EDGE_PID);
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
        USB_FreeReportInfo(info);
        auto intf = std::make_shared<Ps5Host>(dev_addr, itf_desc->bInterfaceNumber, list->m_id);
        intf->m_third_party = isThirdParty;
        if (isThirdParty)
        {
            // request capabilities for 3rd party gamepad
            intf->send_ctrl_xfer(setup_input_caps, data, nullptr);
            ps5_parse_capabilities(data, sizeof(data),
                                   intf->m_subtype, intf->m_sensors_supported,
                                   intf->m_lightbar_supported, intf->m_vibration_supported,
                                   intf->m_touchpad_supported);
        }
        else
        {
            // everything supported on first party controller
            intf->m_sensors_supported = true;
            intf->m_lightbar_supported = true;
            intf->m_vibration_supported = true;
            intf->m_touchpad_supported = true;
            intf->m_subtype = Gamepad;
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
            printf("%02x, %02x, %02x, %02x\r\n", TUSB_DESC_ENDPOINT, desc_ep->bDescriptorType, desc_ep->bEndpointAddress, desc_ep->wMaxPacketSize);
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
        printf("ps5 host found!\r\n");
        
        // Register as auth provider for official PS5 controller
        if (!auth_broker.has_handler(ModePs5) && vid == 0x2b81 && pid == 0x0101)
        {
            auth_broker.register_handler(ModePs5, [intf](XGIPProtocol* packet) {
                // PS5 doesn't use XGIP, this is just for interface compatibility
            });
            // Also register the device itself for HID feature report auth
            auth_broker.register_auth_device(ModePs5, intf);
            intf->m_auth_registered = true;
        }
        else
        {
            usb_host_add_assignable_interface(intf);
        }
        USB_FreeReportInfo(info);
        return intf;
    }
    return nullptr;
}

bool Ps5Host::send_intr_report(const void *buffer, uint8_t len)
{
    TU_VERIFY(usbh_edpt_claim(m_dev_addr, m_ep_out));
    if (!usbh_edpt_xfer(m_dev_addr, m_ep_out, (uint8_t *)buffer, len))
    {
        usbh_edpt_release(m_dev_addr, m_ep_out);
        return false;
    }
    return true;
}

bool Ps5Host::get_intr_report(void *buffer, uint8_t len)
{
    if (!received_packet)
    {
        return false;
    }
    memcpy(buffer, m_ep_in_buf, TU_MIN(len, m_ep_in_size));
    received_packet = false;
    return true;
}

bool Ps5Host::set_config()
{
    UsbHostInterface::set_config();
    return true;
}

bool Ps5Host::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        received_packet = true;
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool ps5_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output& type)
{
    PS5Dpad_Data_t *report = (PS5Dpad_Data_t *)buf;
    uint8_t dpad = report->dpad >= 0x08 ? 0 : HidHost::dpad_bindings_reverse[report->dpad];
    asm volatile("" ::
                     : "memory");
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (PS5Gamepad_Data_t *)buf;
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
    switch (subtype)
    {
    case RockBandGuitar:
        if (type.which_mapping == proto_Output_rbButton_tag)
        {
            auto data = (PS5RockBandGuitar_Data_t *)buf;
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
            auto data = (PS5GHLGuitar_Data_t *)buf;
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

uint16_t ps5_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output& type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (PS5Gamepad_Data_t *)buf;
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
    switch (subtype)
    {
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (PS5GHLGuitar_Data_t *)buf;
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
            auto data = (PS5RockBandGuitar_Data_t *)buf;
            switch (type.mapping.rbAxis)
            {
            case RockBandGuitar_Whammy:
                return data->whammy << 8;
            case RockBandGuitar_Tilt:
                return data->tilt << 8;
            case RockBandGuitar_Pickup:
                return data->pickup << 8;
            default:
                return 0;
            }
        }
    default:
        break;
    }

    return 0;
}

bool Ps5Host::tick_digital(proto_Output& type)
{
    return ps5_tick_digital(m_ep_in_buf, m_subtype, false, type);
}

uint16_t Ps5Host::tick_analog(proto_Output& type)
{
    return ps5_tick_analog(m_ep_in_buf, m_subtype, false, type);
}

bool Ps5Host::send_ps5_output()
{
    ps5_output_report rep = {};
    rep.report_id = 0x02;
    rep.vibration_flag = 1;
    rep.light_bar_flag = 1;
    rep.player_indicator_flag = 1;
    rep.motor_left = m_rumble_left;
    rep.motor_right = m_rumble_right;
    rep.lightbar_red = m_lightbar_r;
    rep.lightbar_green = m_lightbar_g;
    rep.lightbar_blue = m_lightbar_b;
    rep.player_indicator = m_player_indicator;

    if (m_ep_out)
    {
        return send_intr_report(&rep, sizeof(rep));
    }
    return set_report(rep.report_id, HID_REPORT_TYPE_OUTPUT, (uint8_t *)&rep, sizeof(rep));
}

void Ps5Host::set_rumble(uint8_t left, uint8_t right)
{
    m_rumble_left = left;
    m_rumble_right = right;
    send_ps5_output();
}

void Ps5Host::set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    m_lightbar_r = r;
    m_lightbar_g = g;
    m_lightbar_b = b;
    send_ps5_output();
}

void Ps5Host::set_player_led(uint8_t player)
{
    static const uint8_t ps5_leds[] = {0x00, 0x04, 0x0A, 0x15, 0x1B};
    m_player_indicator = (player <= 4) ? ps5_leds[player] : 0;
    send_ps5_output();
}