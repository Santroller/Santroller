#include "tusb_option.h"
#include "devices/usb/host/hid/ps3_host.h"
#include "class/hid/hid.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "managers/device_manager.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/config_manager.hpp"
#include "config/device_factory.hpp"
#include "hidparser.h"
#include "protocols/ps3.hpp"
#include "protocols/pro_keys.hpp"
#include "devices/usb/host/gh_slider_helpers.h"

std::shared_ptr<UsbHostInterface> Ps3Host::open(std::shared_ptr<UsbHostDevice> list, tusb_desc_interface_t const *itf_desc, uint16_t max_len, uint16_t vid, uint16_t pid, uint16_t revision, HID_ReportInfo_t *info)
{
    uint8_t dev_addr = list->dev_addr();

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    bool isThirdParty = info ? info->foundPS3Usage : false;
    bool isValid = isThirdParty;
    bool rb2 = false;
    bool ion = false;
    SubType subtype = Gamepad;
    switch (vid)
    {
    case SWITCH_ARCADE_VID:
        if (pid == SWITCH_ARCADE_PID || pid == SWITCH_TATACON_PID)
        {
            isValid = true;
            isThirdParty = true;
            subtype = pid == SWITCH_TATACON_PID ? Taiko : ProjectDiva;
        }
        break;
    case PS3_DANCEPAD_VID:
        if (pid == PS3_DANCEPAD_PID)
        {
            isValid = true;
            isThirdParty = true;
            subtype = Dancepad;
        }
        break;
    case SONY_VID:
        switch (pid)
        {
        case SONY_DS3_PID:
            isValid = true;
            isThirdParty = false;
            break;
        }
        break;
    case REDOCTANE_VID:
        isThirdParty = true;
        switch (pid)
        {
        case PS3_GH_GUITAR_PID:
            subtype = GuitarHeroGuitar;
            isValid = true;
            break;
        case PS3_GH_DRUM_PID:
            subtype = GuitarHeroDrums;
            isValid = true;
            break;
        case PS3_RB_GUITAR_PID:
            subtype = RockBandGuitar;
            isValid = true;
            break;
        case PS3_MPA_DRUM_PID:
            rb2 = true;
            subtype = RockBandDrums;
            isValid = true;
            break;
        case PS3_RB_DRUM_PID:
            rb2 = revision != 0x1000;
            subtype = RockBandDrums;
            isValid = true;
            break;
        case PS3_DJ_TURNTABLE_PID:
            subtype = DjHeroTurntable;
            isValid = true;
            break;
        case PS3WIIU_GHLIVE_DONGLE_PID:
            subtype = LiveGuitar;
            isValid = true;
            break;
        case PS3_MPA_KEYBOARD_PID:
        case PS3_KEYBOARD_PID:
            subtype = ProKeys;
            isValid = true;
            break;
        case PS3_MUSTANG_PID:
        case PS3_MUSTANG_MPA_PID:
            subtype = ProGuitarMustang;
            isValid = true;
            break;
        case PS3_SQUIRE_PID:
        case PS3_SQUIRE_MPA_PID:
            subtype = ProGuitarSquire;
            isValid = true;
            break;
        }
        break;

    case HARMONIX_VID:
        // Polled the same as PS3, so treat them as PS3 instruments
        isThirdParty = true;
        switch (pid)
        {
        case WII_RB_GUITAR_PID:
        case WII_RB_GUITAR_2_PID:
            subtype = RockBandGuitar;
            isValid = true;
            break;

        case WII_RB_DRUM_PID:
            rb2 = false;
            subtype = RockBandDrums;
            isValid = true;
            break;
        case WII_RB_DRUM_2_PID:
        case WII_MPA_DRUMS_PID:
            rb2 = true;
            subtype = RockBandDrums;
            isValid = true;
            break;
        case WII_KEYBOARD_PID:
        case WII_MPA_KEYBOARD_PID:
            subtype = ProKeys;
            isValid = true;
            break;
        case WII_MUSTANG_PID:
        case WII_MUSTANG_MPA_PID:
            subtype = ProGuitarMustang;
            isValid = true;
            break;
        case WII_SQUIRE_PID:
        case WII_SQUIRE_MPA_PID:
            subtype = ProGuitarSquire;
            isValid = true;
            break;
        case XBOX_360_ION_ROCKER_VID:
            rb2 = true;
            ion = true;
            subtype = RockBandDrums;
            isValid = true;
            break;
        }

        break;
    }
    bool wt = false;
    if (subtype == GuitarHeroGuitar)
    {
        CFG_TUSB_MEM_ALIGN uint8_t str_buf[256];
        if (tuh_descriptor_get_product_string_sync(dev_addr, 0, str_buf, sizeof(str_buf)) == XFER_RESULT_SUCCESS)
        {
            uint16_t wtProduct[] = {'G', 'u', 'i', 't', 'a', 'r', ' ', 'H', 'e', 'r', 'o', '4'};
            if (memcmp(wtProduct, str_buf, sizeof(wtProduct)) == 0 ||
                memcmp(wtProduct, str_buf + 1, sizeof(wtProduct)) == 0 ||
                memcmp(wtProduct, (uint16_t *)str_buf + 1, sizeof(wtProduct)) == 0)
            {
                wt = true;
            }
        }
    }
    if (isValid)
    {
        bool dancepad = vid == PS3_DANCEPAD_VID && pid == PS3_DANCEPAD_PID;
        bool switch_arcade = vid == SWITCH_ARCADE_VID &&
            (pid == SWITCH_ARCADE_PID || pid == SWITCH_TATACON_PID);
        bool compact_report = dancepad || switch_arcade;
        if (compact_report && (itf_desc->bInterfaceClass != TUSB_CLASS_HID ||
                               itf_desc->bInterfaceSubClass != HID_SUBCLASS_NONE ||
                               itf_desc->bInterfaceProtocol != HID_ITF_PROTOCOL_NONE ||
                               max_len < itf_desc->bLength + sizeof(tusb_hid_descriptor_hid_t)))
            return nullptr;
        auto intf = std::make_shared<Ps3Host>(dev_addr, itf_desc->bInterfaceNumber, list->m_id, isThirdParty, rb2, ion, wt, subtype);
        intf->m_dancepad = dancepad;
        intf->m_switch_arcade = switch_arcade;

        if (!isThirdParty && vid == SONY_VID && pid == SONY_DS3_PID)
        {
            // Enable PS3 reports
            uint8_t hid_command_enable[] = {0x42, 0x0c, 0x00, 0x00};
            intf->set_report(0xF4, HID_REPORT_TYPE_FEATURE, hid_command_enable, sizeof(hid_command_enable));

            intf->m_output_report.leds_bitmap = 0x02; // LED 1
            intf->m_player = 1;
            intf->set_report(PS3_RUMBLE_ID, HID_REPORT_TYPE_OUTPUT,
                             reinterpret_cast<uint8_t *>(&intf->m_output_report),
                             sizeof(intf->m_output_report));
        }

        if (subtype == ProKeys || subtype == ProGuitarMustang || subtype == ProGuitarSquire)
        {
            intf->enable_pro_instrument_full_report();
            intf->m_last_pro_instrument_poke = millis();
        }

        if (subtype == LiveGuitar)
        {
            uint8_t ghl_ps3wiiu_magic_data[] = {0x02, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00};
            intf->set_report(0x01, HID_REPORT_TYPE_OUTPUT, ghl_ps3wiiu_magic_data, sizeof(ghl_ps3wiiu_magic_data));
            intf->set_report(0x02, HID_REPORT_TYPE_OUTPUT, ghl_ps3wiiu_magic_data, sizeof(ghl_ps3wiiu_magic_data));
            intf->m_last_ghl_poke = millis();
        }

        uint8_t endpoints = itf_desc->bNumEndpoints;
        p_desc = tu_desc_next(p_desc);
        tusb_hid_descriptor_hid_t *x_desc =
            (tusb_hid_descriptor_hid_t *)p_desc;
        TU_VERIFY(HID_DESC_TYPE_HID == x_desc->bDescriptorType, nullptr);
        uint16_t consumed = itf_desc->bLength + x_desc->bLength;
        if (compact_report && (x_desc->bLength < sizeof(tusb_hid_descriptor_hid_t) || consumed > max_len))
            return nullptr;
        while (endpoints--)
        {
            if (compact_report && consumed + sizeof(tusb_desc_endpoint_t) > max_len)
                return nullptr;
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_VERIFY(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType, nullptr);
            if (compact_report && (desc_ep->bLength < sizeof(tusb_desc_endpoint_t) ||
                                   consumed + desc_ep->bLength > max_len ||
                                   desc_ep->bmAttributes.xfer != TUSB_XFER_INTERRUPT))
                return nullptr;
            if (desc_ep->bEndpointAddress & 0x80)
            {
                if (compact_report && (desc_ep->wMaxPacketSize > sizeof(intf->m_ep_in_buf) ||
                                       desc_ep->wMaxPacketSize < (dancepad ? sizeof(PS3Dpad_Data_t) : sizeof(SwitchArcadeReport)) ||
                                       intf->m_ep_in))
                    return nullptr;
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
            consumed += desc_ep->bLength;
        }
        if (compact_report && !intf->m_ep_in)
            return nullptr;
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
    return nullptr;
}
void Ps3Host::set_stagekit_led(uint8_t param, uint8_t command)
{
    if (!has_stagekit_led())
        return;
    std::array<uint8_t, 2> value = {param, command};
    if (!m_stagekit_set || value != m_last_stagekit_command)
    {
        if (m_stagekit_queue_count == stagekit_queue_capacity)
        {
            if (!m_stagekit_queue_overflow_reported)
                printf("PS3 stage-kit queue full: dev=%u\r\n", m_dev_addr);
            m_stagekit_queue_overflow_reported = true;
            return;
        }
        uint8_t tail = (m_stagekit_queue_head + m_stagekit_queue_count) % stagekit_queue_capacity;
        m_stagekit_commands[tail] = value;
        ++m_stagekit_queue_count;
        m_last_stagekit_command = value;
        m_stagekit_set = true;
    }
}
bool Ps3Host::set_config()
{
    UsbHostInterface::set_config();
    if (m_ep_in)
    {
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool Ps3Host::xfer_cb(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (ep_addr & 0x80)
    {
        if (m_dancepad)
        {
            m_valid_dancepad_report = result == XFER_RESULT_SUCCESS &&
                xferred_bytes == sizeof(PS3Dpad_Data_t) &&
                (m_ep_in_buf[2] & 0x0f) <= 8;
            if (!m_valid_dancepad_report)
                printf("PS3 dancepad: invalid input transfer (result %u, length %lu)\n",
                       static_cast<unsigned>(result), static_cast<unsigned long>(xferred_bytes));
        }
        if (m_switch_arcade)
        {
            m_valid_switch_arcade_report = result == XFER_RESULT_SUCCESS &&
                xferred_bytes >= sizeof(SwitchArcadeReport);
            if (m_valid_switch_arcade_report)
            {
                const uint8_t buttons_low = m_ep_in_buf[0];
                // switch arcade report clearly kept things in the same position instead of keeping the icons the same
                // so swap AB and XY
                const uint8_t face_buttons = buttons_low & 0x0f;
                m_ep_in_buf[0] = (buttons_low & 0xf0) |
                    ((face_buttons & 0x01) << 3) |
                    ((face_buttons & 0x02) << 1) |
                    ((face_buttons & 0x04) >> 1) |
                    ((face_buttons & 0x08) >> 3);
                memset(&m_ep_in_buf[7], 0, sizeof(m_ep_in_buf) - 7);
                auto *report = reinterpret_cast<PS3Dpad_Data_t *>(m_ep_in_buf);
                report->leftTrigger = (buttons_low & static_cast<uint8_t>(SwitchArcade_ZL)) ? UINT8_MAX : 0;
                report->rightTrigger = (buttons_low & static_cast<uint8_t>(SwitchArcade_ZR)) ? UINT8_MAX : 0;
            }
            else
                printf("Switch arcade: invalid input transfer (result %u, length %lu)\n",
                       static_cast<unsigned>(result), static_cast<unsigned long>(xferred_bytes));
        }
        if ((m_subtype == ProKeys || m_subtype == ProGuitarMustang || m_subtype == ProGuitarSquire) &&
            (millis() - m_last_pro_instrument_poke) >= 8000)
        {
            enable_pro_instrument_full_report();
            m_last_pro_instrument_poke = millis();
        }
        if (m_subtype == LiveGuitar)
        {
            if ((millis() - m_last_ghl_poke) >= 8000)
            {
                m_last_ghl_poke = millis();
                uint8_t ghl_ps3wiiu_magic_data[] = {0x02, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00};
                set_report(0x02, HID_REPORT_TYPE_OUTPUT, ghl_ps3wiiu_magic_data, sizeof(ghl_ps3wiiu_magic_data));
            }
        }
        usbh_edpt_xfer(m_dev_addr, m_ep_in, m_ep_in_buf, m_ep_in_size);
    }
    return true;
}

bool ps3_tick_digital(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type, bool wt)
{
    const uint8_t *m_ep_in_buf = buf;
    if (!third_party)
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
        if (type.which_mapping == proto_Output_gamepadAxis_tag)
        {
            auto data = (PS3Gamepad_Data_t *)m_ep_in_buf;
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftTrigger: return data->l2;
            case Gamepad_RightTrigger: return data->r2;
            default: break;
            }
        }
        return false;
    }
    PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)m_ep_in_buf;
    uint8_t dpad = report->dpad >= 0x08 ? 0 : HidHost::dpad_bindings_reverse[report->dpad];
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
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        auto data = (PS3ThirdPartyGamepad_Data_t *)m_ep_in_buf;
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftTrigger: return data->l2;
        case Gamepad_RightTrigger: return data->r2;
        default: break;
        }
    }
    switch (subtype)
    {
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghButton_tag)
        {
            auto data = (PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf;
            uint8_t frets = wt ? decode_ghwt_slider(data->slider) : decode_gh5_slider(data->slider);
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
            case GuitarHeroGuitar_Pedal:
                return data->rightShoulder;
            case GuitarHeroGuitar_TapGreen:
                return frets & 0b00001;
            case GuitarHeroGuitar_TapRed:
                return frets & 0b00010;
            case GuitarHeroGuitar_TapYellow:
                return frets & 0b00100;
            case GuitarHeroGuitar_TapBlue:
                return frets & 0b01000;
            case GuitarHeroGuitar_TapOrange:
                return frets & 0b10000;
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
    case ProGuitarSquire:
    case ProGuitarMustang:
        if (type.which_mapping == proto_Output_proButton_tag &&
            type.mapping.proButton == ProGuitar_Pedal)
        {
            const auto *data = reinterpret_cast<const PS3RockBandProGuitar_Data_t *>(m_ep_in_buf);
            return data->pedal;
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
            case GuitarHeroLiveGuitar_GHTV:
                return data->leftThumbClick;
            default:
                return false;
            }
        }
        return false;
    case ProKeys:
        if (type.which_mapping == proto_Output_proKeySingle_tag ||
            type.which_mapping == proto_Output_proKeyMultiple_tag)
        {
            const auto *data = reinterpret_cast<const PS3RockBandProKeyboard_Data_t *>(m_ep_in_buf);
            if (type.which_mapping == proto_Output_proKeySingle_tag)
                return pro_keyboard_key_pressed(data->key1, data->key2, data->key3,
                                                data->velocities, type.mapping.proKeySingle);
            return pro_keyboard_any_key_pressed(data->key1, data->key2, data->key3,
                                                data->velocities, type.mapping.proKeyMultiple);
        }
        if (type.which_mapping == proto_Output_proKeyboardButton_tag)
            {
                const auto *data = reinterpret_cast<const PS3RockBandProKeyboard_Data_t *>(m_ep_in_buf);
                switch (type.mapping.proKeyboardButton)
                {
                case ProKeyboardOverdrive:
                    return data->overdrive;
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
uint16_t ps3_tick_button_pressure(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type)
{
    if (!ps3_tick_digital(buf, subtype, third_party, type))
    {
        return 0;
    }
    if (subtype == ProKeys &&
        (type.which_mapping == proto_Output_proKeySingle_tag ||
         type.which_mapping == proto_Output_proKeyMultiple_tag))
    {
        const auto *data = reinterpret_cast<const PS3RockBandProKeyboard_Data_t *>(buf);
        if (type.which_mapping == proto_Output_proKeySingle_tag)
            return pro_keyboard_key_pressure(data->key1, data->key2, data->key3,
                                             data->velocities, type.mapping.proKeySingle);
        return pro_keyboard_key_range_pressure(data->key1, data->key2, data->key3,
                                               data->velocities, type.mapping.proKeyMultiple);
    }
    if (third_party || type.which_mapping != proto_Output_gamepadButton_tag)
    {
        return UINT16_MAX;
    }

    const auto *data = (const PS3Gamepad_Data_t *)buf;
    switch (type.mapping.gamepadButton)
    {
    case Gamepad_A: return data->pressureCross << 8;
    case Gamepad_B: return data->pressureCircle << 8;
    case Gamepad_X: return data->pressureSquare << 8;
    case Gamepad_Y: return data->pressureTriangle << 8;
    case Gamepad_LeftShoulder: return data->pressureL1 << 8;
    case Gamepad_RightShoulder: return data->pressureR1 << 8;
    case Gamepad_DpadUp: return data->pressureDpadUp << 8;
    case Gamepad_DpadDown: return data->pressureDpadDown << 8;
    case Gamepad_DpadLeft: return data->pressureDpadLeft << 8;
    case Gamepad_DpadRight: return data->pressureDpadRight << 8;
    default: return UINT16_MAX;
    }
}

uint16_t ps3_tick_analog(const uint8_t *buf, SubType subtype, bool third_party, proto_Output &type)
{
    const uint8_t *m_ep_in_buf = buf;
    if (!third_party)
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
                return (UINT8_MAX - data->leftStickY) * 0x101u;
            case Gamepad_RightStickX:
                return data->rightStickX << 8;
            case Gamepad_RightStickY:
                return (UINT8_MAX - data->rightStickY) * 0x101u;
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
    case GuitarHeroGuitar:
        if (type.which_mapping == proto_Output_ghAxis_tag)
        {
            auto data = (PS3GuitarHeroGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.ghAxis)
            {
            case GuitarHeroGuitar_Whammy:
                return data->whammy << 8;
            case GuitarHeroGuitar_Tilt:
                // tilt is inverted on the PS3, so we need to invert it here 
                if (data->tilt < 0x180)
                {
                    return 0xFFFF;
                }
                if (data->tilt > 0x280)
                {
                    return 0;
                }
                return 65535 - ((data->tilt - 0x180) << 8);
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
                return data->tilt << 8;
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
                return data->pickup << 8;
            default:
                return 0;
            }
        }

    case ProGuitarSquire:
    case ProGuitarMustang:
        if (type.which_mapping == proto_Output_proAxis_tag)
        {
            auto data = (PS3RockBandProGuitar_Data_t *)m_ep_in_buf;
            switch (type.mapping.proAxis)
            {
            case ProGuitar_LowEFret:
                return data->lowEFret << 11;
            case ProGuitar_AFret:
                return data->aFret << 11;
            case ProGuitar_DFret:
                return data->dFret << 11;
            case ProGuitar_GFret:
                return data->gFret << 11;
            case ProGuitar_BFret:
                return data->bFret << 11;
            case ProGuitar_HighEFret:
                return data->highEFret << 11;
            case ProGuitar_LowEFretVelocity:
                return data->lowEFretVelocity << 9;
            case ProGuitar_AFretVelocity:
                return data->aFretVelocity << 9;
            case ProGuitar_DFretVelocity:
                return data->dFretVelocity << 9;
            case ProGuitar_GFretVelocity:
                return data->gFretVelocity << 9;
            case ProGuitar_BFretVelocity:
                return data->bFretVelocity << 9;
            case ProGuitar_HighEFretVelocity:
                return data->highEFretVelocity << 9;
            case ProGuitar_Tilt:
                return data->tilt << 9;
            case ProGuitar_AutoCalibrationMicrophone:
                return data->autoCal_Microphone << 8;
            case ProGuitar_AutoCalibrationLight:
                return data->autoCal_Light << 8;
            default:
                return 0;
            }
        }
        break;
    case ProKeys:
        if (type.which_mapping == proto_Output_proKeyboardAxis_tag)
        {
            auto data = (PS3RockBandProKeyboard_Data_t *)m_ep_in_buf;
            switch (type.mapping.proKeyboardAxis)
            {
            case ProKeyboardPedal:
                return (0x7f - data->pedalAnalog) << 9;
            case ProKeyboardTouchPad:
                return data->touchPad << 9;
            default:
                return 0;
            }
        }
        break;
    default:
        break;
    }

    return 0;
}

bool Ps3Host::tick_digital(proto_Output &type)
{
    if (m_switch_arcade && !m_valid_switch_arcade_report)
        return false;
    if (m_dancepad && !m_valid_dancepad_report)
        return false;
    return ps3_tick_digital(m_ep_in_buf, m_subtype, m_third_party, type, m_wt);
}

uint16_t Ps3Host::tick_analog(proto_Output &type)
{
    if (m_switch_arcade && !m_valid_switch_arcade_report)
        return 0;
    if (m_dancepad && !m_valid_dancepad_report)
        return 0;
    return ps3_tick_analog(m_ep_in_buf, m_subtype, m_third_party, type);
}

bool Ps3Host::tick_axis_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag &&
        (type.mapping.gamepadAxis == Gamepad_LeftTrigger || type.mapping.gamepadAxis == Gamepad_RightTrigger))
    {
        return tick_digital(type);
    }
    return UsbHostInterface::tick_axis_digital(type);
}

uint16_t Ps3Host::tick_button_pressure(proto_Output &type)
{
    if (m_switch_arcade && !m_valid_switch_arcade_report)
        return 0;
    if (m_dancepad && !m_valid_dancepad_report)
        return 0;
    return ps3_tick_button_pressure(m_ep_in_buf, m_subtype, m_third_party, type);
}

void Ps3Host::update(bool full_poll, bool send_events)
{
    UsbHostInterface::update(full_poll, send_events);
    if (!m_third_party && !m_bt_pair_done)
    {
        // Point a wired DS3 at us so pressing PS later connects it over bluetooth.
        // Keeps checking until the BT stack is up, as it can start after USB enumerates.
        uint8_t addr[6];
        // the pairing store can't be written mid-reinit, so wait that out too
        if (!ConfigManager::instance().get_reinit_time() &&
            BluetoothStack::instance().local_address(addr))
        {
            pair_ds3_bluetooth(addr);
            m_bt_pair_done = true;
        }
    }
    if (m_third_party && m_player_led_dirty && !m_switch_arcade)
    {
        if (!send_ps3_player_led())
        {
            if (!m_out_submit_failed)
                printf("PS3 player LED submission failed: dev=%u ep=%u\r\n", m_dev_addr, m_ep_out);
            m_out_submit_failed = true;
            return;
        }
        m_out_submit_failed = false;
        m_player_led_dirty = false;
        return;
    }

    if (m_output_dirty && !m_dancepad && !m_switch_arcade &&
        (m_stagekit_queue_count == 0 || m_last_output_stagekit))
    {
        if (!send_ps3_output())
        {
            if (!m_out_submit_failed)
                printf("PS3 output submission failed: dev=%u ep=%u\r\n", m_dev_addr, m_ep_out);
            m_out_submit_failed = true;
            return;
        }
        m_out_submit_failed = false;
        m_output_dirty = false;
        m_last_output_stagekit = false;
        return;
    }
    if (m_stagekit_queue_count != 0)
    {
        const auto &command = m_stagekit_commands[m_stagekit_queue_head];
        uint8_t packet[] = {PS3_RUMBLE_ID, SANTROLLER_LED_ID, command[0], command[1], 0x00};
        if (set_report(PS3_RUMBLE_ID, HID_REPORT_TYPE_OUTPUT, packet, sizeof(packet)) != sizeof(packet))
        {
            if (!m_out_submit_failed)
                printf("PS3 stage-kit submission failed: dev=%u\r\n", m_dev_addr);
            m_out_submit_failed = true;
            return;
        }
        m_out_submit_failed = false;
        m_last_output_stagekit = true;
        complete_stagekit_command();
    }
}

void Ps3Host::complete_stagekit_command()
{
    if (m_stagekit_queue_count == 0)
        return;
    m_stagekit_queue_head = (m_stagekit_queue_head + 1) % stagekit_queue_capacity;
    --m_stagekit_queue_count;
    if (m_stagekit_queue_count < stagekit_queue_capacity)
        m_stagekit_queue_overflow_reported = false;
}

bool Ps3Host::enable_pro_instrument_full_report()
{
    uint8_t report[] = {
        0xE9, 0x00, 0x89, 0x1B, 0x00, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
        0x00, 0x00, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xE9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return set_report(0x00, HID_REPORT_TYPE_FEATURE, report, sizeof(report)) == sizeof(report);
}

bool Ps3Host::pair_ds3_bluetooth(const uint8_t addr[6])
{
    // Feature report 0xF5 holds the DS3's master address: 01 00 <BD_ADDR, MSB first>
    // (same layout sixpair uses). Only write it if it differs, so replugging doesn't
    // rewrite the controller's flash every time.
    // Feature report 0xF2 has the DS3's own BD_ADDR at bytes 4-9. Record it as a DS3 so
    // the bluetooth side lets it connect without authentication (it never pairs).
    uint8_t info[17] = {};
    if (get_report(0xF2, HID_REPORT_TYPE_FEATURE, info, sizeof(info)) == sizeof(info))
    {
        const uint8_t *mac = &info[4];
        DeviceFactory::BluetoothPairingStateData existing = {};
        if (!DeviceFactory::find_bluetooth_pairing_state_by_mac(mac, existing) ||
            existing.vid != SONY_VID || existing.pid != SONY_DS3_PID)
        {
            int32_t id = DeviceFactory::find_bluetooth_pairing_id_by_mac(mac);
            if (id < 0)
            {
                id = DeviceFactory::allocate_bluetooth_pairing_id();
            }
            update_aux_bluetooth_pairing(id, mac, "PLAYSTATION(R)3 Controller", false, Gamepad,
                                         BtControllerType_BtControllerTypePS3, SONY_VID, SONY_DS3_PID, nullptr);
        }
    }

    uint8_t current[8] = {};
    if (get_report(0xF5, HID_REPORT_TYPE_FEATURE, current, sizeof(current)) == sizeof(current) &&
        memcmp(&current[2], addr, 6) == 0)
    {
        return true;
    }
    uint8_t pair[8] = {0x01, 0x00, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]};
    bool ok = set_report(0xF5, HID_REPORT_TYPE_FEATURE, pair, sizeof(pair)) == sizeof(pair);
    printf("DS3 bluetooth pairing to %02X:%02X:%02X:%02X:%02X:%02X %s\r\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], ok ? "ok" : "failed");
    return ok;
}

bool Ps3Host::submit_ps3_output(uint8_t report_id, const void *report, uint8_t len)
{
    memcpy(m_ep_out_buf, report, len);
    uint32_t result = set_report(report_id, HID_REPORT_TYPE_OUTPUT, m_ep_out_buf, len);
    return result == len;
}

bool Ps3Host::send_ps3_player_led()
{
    PS3InstrumentOutput report = {};
    report.output_type = 0x01;
    report.data_length = 0x08;
    report.player_led = m_player >= 1 && m_player <= 4
                            ? static_cast<uint8_t>(1u << (m_player - 1))
                            : 0;
    return submit_ps3_output(PS3_LED_ID, &report, sizeof(report));
}

bool Ps3Host::send_ps3_output()
{
    if (m_switch_arcade)
        return false;
    if (m_subtype == DjHeroTurntable)
    {
        ps3_turntable_output_report_t rep = {};
        rep.outputType = 0x91;
        rep.enable = m_euphoria ? 1 : 0;
        return submit_ps3_output(rep.outputType, &rep, sizeof(rep));
    }

    m_output_report.rumble.right_motor_on = m_rumble_right ? 1 : 0;
    m_output_report.rumble.left_motor_force = m_rumble_left;
    m_output_report.leds_bitmap = ps3_leds_bitmap_for_player(m_player);
    // only the DS3 numbers this report, and only via wValue
    return submit_ps3_output(m_third_party ? 0 : PS3_RUMBLE_ID, &m_output_report, sizeof(m_output_report));
}

void Ps3Host::set_rumble(uint8_t left, uint8_t right)
{
    if (m_rumble_left != left || m_rumble_right != right)
        m_output_dirty = true;
    m_rumble_left = left;
    m_rumble_right = right;
}

void Ps3Host::set_player_led(uint8_t player)
{
    if (m_player != player)
    {
        if (m_third_party && !m_switch_arcade)
            m_player_led_dirty = true;
        else
            m_output_dirty = true;
    }
    m_player = player;
}

void Ps3Host::set_euphoria_led(bool state)
{
    if (m_euphoria != state && m_subtype == DjHeroTurntable)
        m_output_dirty = true;
    m_euphoria = state;
}