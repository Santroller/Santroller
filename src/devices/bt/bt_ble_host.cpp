#include "devices/bt/bt_ble_host.hpp"
#include "devices/bt/bt_host.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

#include <memory>

#include "ble/gatt-service/hids_host.h"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "protocols/hid.hpp"
#include "protocols/xinput.hpp"
#include "protocols/santroller_v1.hpp"

#define XBOX_SERIES_VID  0x045E
#define XBOX_SERIES_PID  0x0B13   // Xbox Series X/S BLE PID

#define ARDWIINO_VID     0x1209
#define ARDWIINO_PID     0x2882
#define ARDWIINO_PID_BLE 0x2885

// ============================================================================
// BleGenericHost (HID-over-GATT / HIDS fallback & Xbox Series BLE)
// ============================================================================

BleGenericHost::~BleGenericHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
}

void BleGenericHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    m_data = {};
    fill_generic_report(m_info, m_report_buf, &m_data);
}

bool BleGenericHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (m_data.genericButtons & (1 << 0)) != 0;
        case Gamepad_B:               return (m_data.genericButtons & (1 << 1)) != 0;
        case Gamepad_X:               return (m_data.genericButtons & (1 << 2)) != 0;
        case Gamepad_Y:               return (m_data.genericButtons & (1 << 3)) != 0;
        case Gamepad_LeftShoulder:    return (m_data.genericButtons & (1 << 4)) != 0;
        case Gamepad_RightShoulder:   return (m_data.genericButtons & (1 << 5)) != 0;
        case Gamepad_Back:            return (m_data.genericButtons & (1 << 6)) != 0;
        case Gamepad_Start:           return (m_data.genericButtons & (1 << 7)) != 0;
        case Gamepad_LeftThumbClick:  return (m_data.genericButtons & (1 << 8)) != 0;
        case Gamepad_RightThumbClick: return (m_data.genericButtons & (1 << 9)) != 0;
        case Gamepad_Guide:           return (m_data.genericButtons & (1 << 10)) != 0;
        case Gamepad_Capture:         return (m_data.genericButtons & (1 << 11)) != 0;
        case Gamepad_DpadUp:          return m_data.dpadUp != 0;
        case Gamepad_DpadDown:        return m_data.dpadDown != 0;
        case Gamepad_DpadLeft:        return m_data.dpadLeft != 0;
        case Gamepad_DpadRight:       return m_data.dpadRight != 0;
        default:                      return false;
        }
    }
    return false;
}

uint16_t BleGenericHost::tick_analog(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:   return m_data.genericAxisX;
        case Gamepad_LeftStickY:   return m_data.genericAxisY;
        case Gamepad_RightStickX:  return m_data.genericAxisRx;
        case Gamepad_RightStickY:  return m_data.genericAxisRy;
        case Gamepad_LeftTrigger:  return m_data.genericAxisZ;
        case Gamepad_RightTrigger: return m_data.genericAxisRz;
        default:                   return 0;
        }
    }
    return 0;
}

// ============================================================================
// BleGhlIosHost (Guitar Hero Live iOS BLE Guitar)
// ============================================================================

bool BleGhlIosHost::tick_digital(proto_Output &type)
{
    uint8_t frets   = m_report_buf[0];
    uint8_t buttons = m_report_buf[1];
    uint8_t strum   = m_report_buf[4];

    if (type.which_mapping == proto_Output_ghlButton_tag)
    {
        switch (type.mapping.ghlButton)
        {
        case GuitarHeroLiveGuitar_Black1:    return (frets & 0x02) != 0;
        case GuitarHeroLiveGuitar_Black2:    return (frets & 0x04) != 0;
        case GuitarHeroLiveGuitar_Black3:    return (frets & 0x08) != 0;
        case GuitarHeroLiveGuitar_White1:    return (frets & 0x01) != 0;
        case GuitarHeroLiveGuitar_White2:    return (frets & 0x10) != 0;
        case GuitarHeroLiveGuitar_White3:    return (frets & 0x20) != 0;
        case GuitarHeroLiveGuitar_StrumUp:   return strum == 0x00;
        case GuitarHeroLiveGuitar_StrumDown: return strum == 0xFF;
        case GuitarHeroLiveGuitar_GHTV:      return (buttons & 0x04) != 0;
        default:                             return false;
        }
    }
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (frets & 0x02) != 0; // Black 1
        case Gamepad_B:               return (frets & 0x04) != 0; // Black 2
        case Gamepad_Y:               return (frets & 0x08) != 0; // Black 3
        case Gamepad_X:               return (frets & 0x01) != 0; // White 1
        case Gamepad_LeftShoulder:    return (frets & 0x10) != 0; // White 2
        case Gamepad_RightShoulder:   return (frets & 0x20) != 0; // White 3
        case Gamepad_Start:           return (buttons & 0x02) != 0; // Pause
        case Gamepad_Back:            return (buttons & 0x08) != 0; // Hero Power
        case Gamepad_LeftThumbClick:  return (buttons & 0x04) != 0; // GHTV
        case Gamepad_Guide:           return (buttons & 0x10) != 0; // Sync
        case Gamepad_DpadUp:          return strum == 0x00;
        case Gamepad_DpadDown:        return strum == 0xFF;
        default:                      return false;
        }
    }
    return false;
}

uint16_t BleGhlIosHost::tick_analog(proto_Output &type)
{
    uint8_t tilt   = m_report_buf[6];
    uint8_t whammy = m_report_buf[19];

    if (type.which_mapping == proto_Output_ghlAxis_tag)
    {
        switch (type.mapping.ghlAxis)
        {
        case GuitarHeroLiveGuitar_Tilt:   return (uint16_t)tilt * 0x101;
        case GuitarHeroLiveGuitar_Whammy: return (uint16_t)whammy * 0x101;
        default:                          return 0;
        }
    }
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_RightStickY: return (uint16_t)tilt * 0x101;
        case Gamepad_RightStickX: return (uint16_t)whammy * 0x101;
        default:                  return 0;
        }
    }
    return 0;
}

// ============================================================================
// BleSantrollerHost (Santroller 1 & 2 BLE Host)
// ============================================================================

BleSantrollerHost::BleSantrollerHost(uint16_t id, bool is_v2, uint16_t version, SubType known_subtype)
    : BluetoothHostInterface(id), m_is_v2(is_v2)
{
    if (m_is_v2)
    {
        if (known_subtype != SubType_Unknown)
        {
            m_subtype = known_subtype;
            m_ready = true;
        }
        else
        {
            m_subtype = SubType_Unknown;
            m_ready = false;
        }
    }
    else
    {
        // Santroller 1: SubType is encoded in version high byte: (version >> 8) & 0xFF
        uint8_t st = (version >> 8) & 0xFF;
        if (known_subtype != SubType_Unknown)
        {
            m_subtype = known_subtype;
        }
        else if (st != 0)
        {
            m_subtype = (SubType)st;
        }
        else
        {
            m_subtype = SubType_Gamepad;
        }
        m_ready = true;
    }
}

void BleSantrollerHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    if (m_is_v2)
    {
        if (m_subtype != SubType_Unknown)
        {
            set_ready(true);
        }
        else
        {
            // Query Santroller capabilities (Report 0x10) over GATT
            hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
            const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
            hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
        }
    }
    else
    {
        set_ready(true);
    }
}

void BleSantrollerHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (m_is_v2)
    {
        handle_report_v2(data, len);
    }
    else
    {
        handle_report_v1(data, len);
    }
}

void BleSantrollerHost::handle_report_v2(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;

    if (data[0] == ReportIdSantrollerCapabilities)
    {
        if (len >= 2)
        {
            m_subtype = (SubType)data[1];
            if (len >= 3)
            {
                m_capabilities = data[2];
            }
            if (!m_ready)
            {
                set_ready(true);
            }
        }
        return;
    }

    if (data[0] != 1) return;

    if (!m_ready)
    {
        if (++m_query_attempts % 30 == 1)
        {
            hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
            const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
            hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
        }
        return;
    }

    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);

    // If not Dancepad, convert hat (bits 0..3 of data[2]) to XInput dpad bitmask
    if (m_subtype != Dancepad && copy_len >= 3)
    {
        uint8_t hat = data[2] & 0x0F;
        uint8_t dpad_bits = 0;
        switch (hat)
        {
        case 0: dpad_bits = 0x01; break; // Up
        case 1: dpad_bits = 0x09; break; // Up + Right
        case 2: dpad_bits = 0x08; break; // Right
        case 3: dpad_bits = 0x0A; break; // Down + Right
        case 4: dpad_bits = 0x02; break; // Down
        case 5: dpad_bits = 0x06; break; // Down + Left
        case 6: dpad_bits = 0x04; break; // Left
        case 7: dpad_bits = 0x05; break; // Up + Left
        default: dpad_bits = 0; break;
        }
        m_report_buf[2] = (m_report_buf[2] & 0xF0) | dpad_bits;
    }
}

void BleSantrollerHost::handle_report_v1(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);
}

bool BleSantrollerHost::tick_digital(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_digital_impl(m_report_buf, m_subtype, type);
    }
    return tick_digital_v1(type);
}

uint16_t BleSantrollerHost::tick_analog(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_analog_impl(m_report_buf, m_subtype, type);
    }
    return tick_analog_v1(type);
}

bool BleSantrollerHost::tick_digital_v1(proto_Output &type)
{
    uint8_t hat = m_report_buf[3] & 0x0F;
    bool up    = (hat == 0 || hat == 1 || hat == 7);
    bool down  = (hat == 3 || hat == 4 || hat == 5);
    bool left  = (hat == 5 || hat == 6 || hat == 7);
    bool right = (hat == 1 || hat == 2 || hat == 3);

    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (m_subtype)
        {
        case Dancepad:
        {
            auto data = (const Santroller1Gamepad_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_DpadUp:    return (data->dpad & 1) != 0;
            case Gamepad_DpadDown:  return ((data->dpad >> 1) & 1) != 0;
            case Gamepad_DpadLeft:  return ((data->dpad >> 2) & 1) != 0;
            case Gamepad_DpadRight: return ((data->dpad >> 3) & 1) != 0;
            case Gamepad_Back:      return data->back;
            case Gamepad_Start:     return data->start;
            default:                return false;
            }
        }
        case GuitarHeroGuitar:
        {
            auto data = (const Santroller1GuitarHeroGuitar_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a;
            case Gamepad_B:             return data->b;
            case Gamepad_X:             return data->x;
            case Gamepad_Y:             return data->y;
            case Gamepad_LeftShoulder:  return data->leftShoulder;
            case Gamepad_RightShoulder: return data->rightShoulder;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case RockBandGuitar:
        {
            auto data = (const Santroller1RockBandGuitar_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a || data->soloGreen;
            case Gamepad_B:             return data->b || data->soloRed;
            case Gamepad_X:             return data->x || data->soloBlue;
            case Gamepad_Y:             return data->y || data->soloYellow;
            case Gamepad_LeftShoulder:  return data->leftShoulder || data->soloOrange;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case LiveGuitar:
        {
            auto data = (const Santroller1GHLGuitar_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:              return data->a;
            case Gamepad_B:              return data->b;
            case Gamepad_X:              return data->x;
            case Gamepad_Y:              return data->y;
            case Gamepad_LeftShoulder:   return data->leftShoulder;
            case Gamepad_RightShoulder:  return data->rightShoulder;
            case Gamepad_Back:           return data->back;
            case Gamepad_Start:          return data->start;
            case Gamepad_LeftThumbClick: return data->leftThumbClick;
            case Gamepad_Guide:          return data->guide;
            case Gamepad_DpadUp:         return up;
            case Gamepad_DpadDown:       return down;
            case Gamepad_DpadLeft:       return left;
            case Gamepad_DpadRight:      return right;
            default:                     return false;
            }
        }
        case GuitarHeroDrums:
        {
            auto data = (const Santroller1GuitarHeroDrums_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a;
            case Gamepad_B:             return data->b;
            case Gamepad_X:             return data->x;
            case Gamepad_Y:             return data->y;
            case Gamepad_LeftShoulder:  return data->leftShoulder;
            case Gamepad_RightShoulder: return data->rightShoulder;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case RockBandDrums:
        {
            auto data = (const Santroller1RockBandDrums_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:             return data->a;
            case Gamepad_B:             return data->b;
            case Gamepad_X:             return data->x;
            case Gamepad_Y:             return data->y;
            case Gamepad_LeftShoulder:  return data->leftShoulder;
            case Gamepad_RightShoulder: return data->rightShoulder;
            case Gamepad_Back:          return data->back;
            case Gamepad_Start:         return data->start;
            case Gamepad_Guide:         return data->guide;
            case Gamepad_DpadUp:        return up;
            case Gamepad_DpadDown:      return down;
            case Gamepad_DpadLeft:      return left;
            case Gamepad_DpadRight:     return right;
            default:                    return false;
            }
        }
        case DjHeroTurntable:
        {
            auto data = (const Santroller1Turntable_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:         return data->a;
            case Gamepad_B:         return data->b;
            case Gamepad_X:         return data->x;
            case Gamepad_Y:         return data->y;
            case Gamepad_Back:      return data->back;
            case Gamepad_Start:     return data->start;
            case Gamepad_Guide:     return data->guide;
            case Gamepad_DpadUp:    return up;
            case Gamepad_DpadDown:  return down;
            case Gamepad_DpadLeft:  return left;
            case Gamepad_DpadRight: return right;
            default:                return false;
            }
        }
        default: // Gamepad and others
        {
            auto data = (const Santroller1Gamepad_Data_t *)m_report_buf;
            switch (type.mapping.gamepadButton)
            {
            case Gamepad_A:               return data->a;
            case Gamepad_B:               return data->b;
            case Gamepad_X:               return data->x;
            case Gamepad_Y:               return data->y;
            case Gamepad_LeftShoulder:    return data->leftShoulder;
            case Gamepad_RightShoulder:   return data->rightShoulder;
            case Gamepad_Back:            return data->back;
            case Gamepad_Start:           return data->start;
            case Gamepad_LeftThumbClick:  return data->leftThumbClick;
            case Gamepad_RightThumbClick: return data->rightThumbClick;
            case Gamepad_Guide:           return data->guide;
            case Gamepad_Capture:         return data->capture;
            case Gamepad_DpadUp:          return up;
            case Gamepad_DpadDown:        return down;
            case Gamepad_DpadLeft:        return left;
            case Gamepad_DpadRight:       return right;
            default:                      return false;
            }
        }
        }
    }

    if (type.which_mapping == proto_Output_ghButton_tag && m_subtype == GuitarHeroGuitar)
    {
        auto data = (const Santroller1GuitarHeroGuitar_Data_t *)m_report_buf;
        switch (type.mapping.ghButton)
        {
        case GuitarHeroGuitar_Green:  return data->a;
        case GuitarHeroGuitar_Red:    return data->b;
        case GuitarHeroGuitar_Yellow: return data->y;
        case GuitarHeroGuitar_Blue:   return data->x;
        case GuitarHeroGuitar_Orange: return data->leftShoulder;
        case GuitarHeroGuitar_Pedal:  return data->rightShoulder;
        default:                      return false;
        }
    }

    if (type.which_mapping == proto_Output_rbButton_tag && m_subtype == RockBandGuitar)
    {
        auto data = (const Santroller1RockBandGuitar_Data_t *)m_report_buf;
        switch (type.mapping.rbButton)
        {
        case RockBandGuitar_Green:      return data->a;
        case RockBandGuitar_Red:        return data->b;
        case RockBandGuitar_Yellow:     return data->y;
        case RockBandGuitar_Blue:       return data->x;
        case RockBandGuitar_Orange:     return data->leftShoulder;
        case RockBandGuitar_SoloGreen:  return data->soloGreen;
        case RockBandGuitar_SoloRed:    return data->soloRed;
        case RockBandGuitar_SoloYellow: return data->soloYellow;
        case RockBandGuitar_SoloBlue:   return data->soloBlue;
        case RockBandGuitar_SoloOrange: return data->soloOrange;
        default:                        return false;
        }
    }

    if (type.which_mapping == proto_Output_ghlButton_tag && m_subtype == LiveGuitar)
    {
        auto data = (const Santroller1GHLGuitar_Data_t *)m_report_buf;
        switch (type.mapping.ghlButton)
        {
        case GuitarHeroLiveGuitar_Black1:    return data->a;
        case GuitarHeroLiveGuitar_Black2:    return data->b;
        case GuitarHeroLiveGuitar_Black3:    return data->y;
        case GuitarHeroLiveGuitar_White1:    return data->x;
        case GuitarHeroLiveGuitar_White2:    return data->leftShoulder;
        case GuitarHeroLiveGuitar_White3:    return data->rightShoulder;
        case GuitarHeroLiveGuitar_StrumUp:   return up;
        case GuitarHeroLiveGuitar_StrumDown: return down;
        default:                             return false;
        }
    }

    if (type.which_mapping == proto_Output_djhButton_tag && m_subtype == DjHeroTurntable)
    {
        auto data = (const Santroller1Turntable_Data_t *)m_report_buf;
        switch (type.mapping.djhButton)
        {
        case DJHTurntable_LeftGreen:  return data->leftGreen;
        case DJHTurntable_LeftRed:    return data->leftRed;
        case DJHTurntable_LeftBlue:   return data->leftBlue;
        case DJHTurntable_RightGreen: return data->rightGreen;
        case DJHTurntable_RightRed:   return data->rightRed;
        case DJHTurntable_RightBlue:  return data->rightBlue;
        default:                      return false;
        }
    }

    return false;
}

uint16_t BleSantrollerHost::tick_analog_v1(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        if (m_subtype == Gamepad)
        {
            auto data = (const Santroller1Gamepad_Data_t *)m_report_buf;
            switch (type.mapping.gamepadAxis)
            {
            case Gamepad_LeftStickX:   return (uint16_t)data->leftStickX * 0x101;
            case Gamepad_LeftStickY:   return (uint16_t)(255 - data->leftStickY) * 0x101;
            case Gamepad_RightStickX:  return (uint16_t)data->rightStickX * 0x101;
            case Gamepad_RightStickY:  return (uint16_t)(255 - data->rightStickY) * 0x101;
            case Gamepad_LeftTrigger:  return (uint16_t)data->leftTrigger * 0x101;
            case Gamepad_RightTrigger: return (uint16_t)data->rightTrigger * 0x101;
            default:                   return 0;
            }
        }
    }

    if (type.which_mapping == proto_Output_ghAxis_tag && m_subtype == GuitarHeroGuitar)
    {
        auto data = (const Santroller1GuitarHeroGuitar_Data_t *)m_report_buf;
        switch (type.mapping.ghAxis)
        {
        case GuitarHeroGuitar_Whammy: return (uint16_t)data->whammy * 0x101;
        case GuitarHeroGuitar_Tilt:   return (uint16_t)data->tilt * 0x101;
        default:                      return 0;
        }
    }

    if (type.which_mapping == proto_Output_rbAxis_tag && m_subtype == RockBandGuitar)
    {
        auto data = (const Santroller1RockBandGuitar_Data_t *)m_report_buf;
        switch (type.mapping.rbAxis)
        {
        case RockBandGuitar_Whammy: return (uint16_t)data->whammy * 0x101;
        case RockBandGuitar_Tilt:   return (uint16_t)data->tilt * 0x101;
        case RockBandGuitar_Pickup: return (uint16_t)data->pickup * 0x101;
        default:                    return 0;
        }
    }

    if (type.which_mapping == proto_Output_ghlAxis_tag && m_subtype == LiveGuitar)
    {
        auto data = (const Santroller1GHLGuitar_Data_t *)m_report_buf;
        switch (type.mapping.ghlAxis)
        {
        case GuitarHeroLiveGuitar_Whammy: return (uint16_t)data->whammy * 0x101;
        case GuitarHeroLiveGuitar_Tilt:   return (uint16_t)data->tilt * 0x101;
        default:                          return 0;
        }
    }

    if (type.which_mapping == proto_Output_djhAxis_tag && m_subtype == DjHeroTurntable)
    {
        auto data = (const Santroller1Turntable_Data_t *)m_report_buf;
        switch (type.mapping.djhAxis)
        {
        case DJHTurntable_LeftVelocity:  return (uint16_t)data->leftTableVelocity * 0x101;
        case DJHTurntable_RightVelocity: return (uint16_t)data->rightTableVelocity * 0x101;
        case DJHTurntable_Crossfader:    return (uint16_t)data->crossfader * 0x101;
        case DJHTurntable_EffectsKnob:   return (uint16_t)data->effectsKnob * 0x101;
        default:                         return 0;
        }
    }

    return 0;
}

// ============================================================================
// Factory
// ============================================================================

std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         const uint8_t *desc,
                                                         uint16_t desc_len,
                                                         SubType known_subtype)
{
    bool is_santroller = (vid == ARDWIINO_VID && (pid == ARDWIINO_PID || pid == ARDWIINO_PID_BLE));
    bool has_v2_usage = false;
    if (desc && desc_len >= 2)
    {
        for (uint16_t i = 0; i < desc_len - 1; i++)
        {
            if ((desc[i] == 0x82 && desc[i + 1] == 0x28) ||
                (desc[i] == 0x85 && desc[i + 1] == ReportIdSantrollerCapabilities))
            {
                has_v2_usage = true;
                break;
            }
        }
    }
    if (has_v2_usage)
    {
        is_santroller = true;
    }

    if (is_santroller)
    {
        auto host = std::make_shared<BleSantrollerHost>(device_id, has_v2_usage, version, known_subtype);
        host->m_vid = vid ? vid : ARDWIINO_VID;
        host->m_pid = pid ? pid : (has_v2_usage ? ARDWIINO_PID : ARDWIINO_PID_BLE);
        return host;
    }

    // Xbox Series X/S / Xbox One BLE controllers (VID 0x045E) and any other
    // standard HID-over-GATT gamepads use BleGenericHost with report parsing via fill_generic_report.
    auto host = std::make_shared<BleGenericHost>(device_id, info);
    if (known_subtype != SubType_Unknown)
    {
        host->m_subtype = known_subtype;
    }
    host->m_vid = vid;
    host->m_pid = pid;
    return host;
}
