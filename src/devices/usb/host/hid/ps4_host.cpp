#include "tusb_option.h"
#include "devices/usb/host/hid/ps4_host.h"
#include "protocols/ps4.hpp"
#include "usb/auth_broker.h"
#include "class/hid/hid.h"
#include "devices/usb.hpp"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "utils.h"

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

bool ps4_parse_capabilities(const uint8_t *data, uint16_t len, uint16_t vid, uint16_t pid,
                            SubType &subtype, bool &sensors, bool &lightbar, bool &vibration, bool &touchpad)
{
    const uint8_t *caps = nullptr;
    if (len >= 6 && (data[2] == 0x27 || data[2] == 0xA7))
    {
        caps = &data[2];
    }
    else if (len >= 4 && (data[0] == 0x27 || data[0] == 0xA7))
    {
        caps = &data[0];
    }
    else if (len >= 5 && (data[1] == 0x27 || data[1] == 0xA7))
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
        if (vid == XBOX_REDOCTANE_VID && pid == PS4_GHLIVE_DONGLE_PID)
        {
            subtype = LiveGuitar;
        }
        else
        {
            subtype = RockBandGuitar;
        }
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

std::shared_ptr<UsbHostInterface> Ps4Host::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool isThirdParty = info ? info->foundPS4Usage : false;
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
            ps4_parse_capabilities(data, sizeof(data), vid, pid,
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
        if (intf->m_subtype == LiveGuitar)
        {
            intf->m_ghl_player_led_dirty = true;
            intf->m_last_ghl_poke = millis();
        }
        usb_host_add_assignable_interface(intf);
        USB_FreeReportInfo(info);
        return intf;
    }
    return nullptr;
}

bool Ps4Host::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool Ps4Host::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr == m_ep_out)
    {
        m_out_result = result;
        m_out_done.store(true, std::memory_order_release);
    }
    if (ep_addr & 0x80)
    {
        if (m_subtype == LiveGuitar)
        {
            if ((millis() - m_last_ghl_poke) >= 8000)
            {
                m_last_ghl_poke = millis();
                m_ghl_player_led_dirty = true;
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool ps4_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, uint32_t *last_ghl_poke)
{
    PS4Dpad_Data_t *report = (PS4Dpad_Data_t *)buf;
    uint8_t dpad = report->dpad >= 0x08 ? 0 : HidHost::dpad_bindings_reverse[report->dpad];
    bool up = dpad & UP;
    bool left = dpad & LEFT;
    bool down = dpad & DOWN;
    bool right = dpad & RIGHT;
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        auto data = (PS4Gamepad_Data_t *)buf;
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
            auto data = (PS4RockBandGuitar_Data_t *)buf;
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
            auto data = (PS4GHLGuitar_Data_t *)buf;
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
            case GuitarHeroLiveGuitar_GHTV:
                return data->leftThumbClick;
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

uint16_t ps4_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (PS4Gamepad_Data_t *)buf;
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftTrigger:
            return data->leftTrigger << 8;
        case Gamepad_RightTrigger:
            return data->rightTrigger << 8;
        case Gamepad_LeftStickX:
            return data->leftStickX << 8;
        case Gamepad_LeftStickY:
            return (UINT8_MAX - data->leftStickY) * 0x101u;
        case Gamepad_RightStickX:
            return data->rightStickX << 8;
        case Gamepad_RightStickY:
            return (UINT8_MAX - data->rightStickY) * 0x101u;
        default:
            return 0;
        }
    }
    switch (subtype)
    {
    case LiveGuitar:
        if (type.which_mapping == proto_Output_ghlAxis_tag)
        {
            auto data = (PS4GHLGuitar_Data_t *)buf;
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
            auto data = (PS4RockBandGuitar_Data_t *)buf;
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

bool Ps4Host::tick_digital(proto_Output &type)
{
    return ps4_tick_digital(m_ep_in_buf, m_subtype, m_third_party, type, &m_last_ghl_poke);
}

uint16_t Ps4Host::tick_analog(proto_Output &type)
{
    return ps4_tick_analog(m_ep_in_buf, m_subtype, m_third_party, type);
}

void Ps4Host::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (m_out_done.exchange(false, std::memory_order_acquire))
    {
        m_out_pending = false;
        if (m_out_result != XFER_RESULT_SUCCESS)
        {
            m_output_dirty = true;
            printf("PS4 output failed: dev=%u ep=%u result=%u\r\n", m_dev_addr, m_ep_out, unsigned(m_out_result));
        }
    }

    if (m_out_pending || (m_ep_out && usbh_edpt_busy(m_dev_addr, m_ep_out)))
        return;

    if (m_subtype == LiveGuitar && m_ghl_player_led_dirty)
    {
        if (!send_ghl_player_led())
        {
            if (!m_ghl_submit_failed)
                printf("PS4 GHL player LED submission failed: dev=%u\r\n", m_dev_addr);
            m_ghl_submit_failed = true;
            return;
        }
        m_ghl_submit_failed = false;
        m_ghl_player_led_dirty = false;
    }

    if (!m_output_dirty || m_out_pending ||
        (m_ep_out && usbh_edpt_busy(m_dev_addr, m_ep_out)))
        return;
    if (!send_ps4_output())
    {
        if (!m_out_submit_failed)
            printf("PS4 output submission failed: dev=%u ep=%u\r\n", m_dev_addr, m_ep_out);
        m_out_submit_failed = true;
        return;
    }
    m_out_submit_failed = false;
    m_output_dirty = false;
    m_out_pending = m_ep_out != 0;
}

bool Ps4Host::send_ghl_player_led()
{
    uint8_t player_led = static_cast<uint8_t>(1u << (m_player - 1));
    uint8_t report[] = {0x30, player_led, 0x08, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
    return set_report(0x2, HID_REPORT_TYPE_OUTPUT, report, sizeof(report)) == sizeof(report);
}

bool Ps4Host::send_ps4_output()
{
    memcpy(m_ep_out_buf, &m_output_report, sizeof(m_output_report));
    if (m_ep_out)
    {
        return send_intr_xfer(m_ep_out, m_ep_out_buf, sizeof(m_output_report));
    }
    return set_report(m_output_report.report_id, HID_REPORT_TYPE_OUTPUT,
                      m_ep_out_buf, sizeof(m_output_report)) == sizeof(m_output_report);
}

void Ps4Host::set_rumble(uint8_t left, uint8_t right)
{
    if (m_output_report.motor_left == left && m_output_report.motor_right == right)
        return;
    m_output_report.motor_left = left;
    m_output_report.motor_right = right;
    m_output_dirty = true;
}

void Ps4Host::set_lightbar(uint8_t r, uint8_t g, uint8_t b)
{
    if (m_output_report.lightbar_red == r &&
        m_output_report.lightbar_green == g &&
        m_output_report.lightbar_blue == b)
        return;
    m_output_report.lightbar_red = r;
    m_output_report.lightbar_green = g;
    m_output_report.lightbar_blue = b;
    m_output_dirty = true;
}

void Ps4Host::set_player_led(uint8_t player)
{
    if (m_subtype == LiveGuitar)
    {
        if (player < 1 || player > 4 || m_player == player)
            return;
        m_player = player;
        m_ghl_player_led_dirty = true;
        return;
    }

    if (player == 0)
    {
        set_lightbar(0, 0, 0);
    }
    else if (player <= 4)
    {
        set_lightbar(ps4_colors[player - 1][0],
                     ps4_colors[player - 1][1],
                     ps4_colors[player - 1][2]);
    }
}