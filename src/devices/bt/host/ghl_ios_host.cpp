#include "devices/bt/host/ghl_ios_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

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
