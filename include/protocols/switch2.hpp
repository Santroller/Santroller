#pragma once
#include <stdint.h>
#include <string.h>
#include "input.pb.h"

// ---------------------------------------------------------------------------
// Switch 2 Controller Protocol Definitions
// ---------------------------------------------------------------------------

struct Switch2ControllerState {
    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;
    bool l = false;
    bool r = false;
    bool zl = false;
    bool zr = false;
    bool minus = false;
    bool plus = false;
    bool l3 = false;
    bool r3 = false;
    bool home = false;
    bool capture = false;
    bool dpad_up = false;
    bool dpad_down = false;
    bool dpad_left = false;
    bool dpad_right = false;
    bool sl = false;
    bool sr = false;

    uint16_t left_stick_x = 2048;
    uint16_t left_stick_y = 2048;
    uint16_t right_stick_x = 2048;
    uint16_t right_stick_y = 2048;
};

static inline bool switch2_parse_report(const uint8_t *buf, uint16_t len, Switch2ControllerState &out)
{
    if (len < 14) return false;

    // Report format:
    // Offset 4..7: Buttons
    // Offset 8..10: Left stick (12-bit packed)
    // Offset 11..13: Right stick (12-bit packed)
    uint32_t buttons = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);

    out.y          = (buttons & (1 << 0)) != 0;
    out.x          = (buttons & (1 << 1)) != 0;
    out.b          = (buttons & (1 << 2)) != 0;
    out.a          = (buttons & (1 << 3)) != 0;
    out.sr         = (buttons & (1 << 4)) != 0;
    out.sl         = (buttons & (1 << 5)) != 0;
    out.r          = (buttons & (1 << 6)) != 0;
    out.zr         = (buttons & (1 << 7)) != 0;
    out.minus      = (buttons & (1 << 8)) != 0;
    out.plus       = (buttons & (1 << 9)) != 0;
    out.r3         = (buttons & (1 << 10)) != 0;
    out.l3         = (buttons & (1 << 11)) != 0;
    out.home       = (buttons & (1 << 12)) != 0;
    out.capture    = (buttons & (1 << 13)) != 0;
    out.dpad_down  = (buttons & (1 << 16)) != 0;
    out.dpad_up    = (buttons & (1 << 17)) != 0;
    out.dpad_right = (buttons & (1 << 18)) != 0;
    out.dpad_left  = (buttons & (1 << 19)) != 0;
    out.l          = (buttons & (1 << 22)) != 0;
    out.zl         = (buttons & (1 << 23)) != 0;

    // 12-bit packed sticks (0..4095)
    out.left_stick_x  = buf[8]  | ((buf[9]  & 0x0F) << 8);
    out.left_stick_y  = ((buf[9] >> 4) & 0x0F) | (buf[10] << 4);
    out.right_stick_x = buf[11] | ((buf[12] & 0x0F) << 8);
    out.right_stick_y = ((buf[12] >> 4) & 0x0F) | (buf[13] << 4);

    return true;
}

static inline bool switch2_tick_digital(const Switch2ControllerState &s, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return s.a;
        case Gamepad_B:               return s.b;
        case Gamepad_X:               return s.x;
        case Gamepad_Y:               return s.y;
        case Gamepad_LeftShoulder:    return s.l || s.sl;
        case Gamepad_RightShoulder:   return s.r || s.sr;
        case Gamepad_Back:            return s.minus;
        case Gamepad_Start:           return s.plus;
        case Gamepad_LeftThumbClick:  return s.l3;
        case Gamepad_RightThumbClick: return s.r3;
        case Gamepad_Guide:           return s.home;
        case Gamepad_Capture:         return s.capture;
        case Gamepad_DpadUp:          return s.dpad_up;
        case Gamepad_DpadDown:        return s.dpad_down;
        case Gamepad_DpadLeft:        return s.dpad_left;
        case Gamepad_DpadRight:       return s.dpad_right;
        default:                      return false;
        }
    }
    return false;
}

static inline uint16_t switch2_tick_analog(const Switch2ControllerState &s, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:   return (uint16_t)(s.left_stick_x << 4);
        case Gamepad_LeftStickY:   return (uint16_t)(s.left_stick_y << 4);
        case Gamepad_RightStickX:  return (uint16_t)(s.right_stick_x << 4);
        case Gamepad_RightStickY:  return (uint16_t)(s.right_stick_y << 4);
        case Gamepad_LeftTrigger:  return s.zl ? 0xFFFF : 0;
        case Gamepad_RightTrigger: return s.zr ? 0xFFFF : 0;
        default:                   return 0;
        }
    }
    return 0;
}
