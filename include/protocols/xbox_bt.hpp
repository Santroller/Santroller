#pragma once
#include <stdint.h>
#include <string.h>
#include "config.pb.h"

// ---------------------------------------------------------------------------
// Xbox One S / Elite 2 / Series controllers over Bluetooth (Classic and BLE)
//
// These are plain HID devices over Bluetooth, not GIP. Every firmware uses input report 0x01:
//   [1..8]   LX, LY, RX, RY  (u16, 0 - 65535, down is the maximum)
//   [9..12]  LT, RT          (u16, 0 - 1023)
//   [13]     hat             (1 - 8 clockwise from up, 0 is neutral)
//   [14..15] buttons
//   [16]     AC Back (View on One S / Elite 2 BLE firmware) or Share on Series controllers
// The original One S firmware (16 byte report) has the buttons packed in order and sends Guide
// in report 0x02. Newer firmware uses the Android gamepad layout (gaps where C / Z / L2 / R2 would be).
// Layouts match SDL's HIDAPI Xbox One Bluetooth driver and Linux hid-microsoft.
// ---------------------------------------------------------------------------

#define XBOX_BT_VID 0x045E
#define XBOX_BT_ONE_S_LEGACY_PID 0x02E0
#define XBOX_BT_ONE_S_PID 0x02FD
#define XBOX_BT_ELITE_2_PID 0x0B05
#define XBOX_BT_SERIES_PID 0x0B13
#define XBOX_BT_ONE_S_BLE_PID 0x0B20
#define XBOX_BT_ELITE_2_BLE_PID 0x0B22

#define XBOX_BT_INPUT_REPORT_ID 0x01
#define XBOX_BT_GUIDE_REPORT_ID 0x02
#define XBOX_BT_RUMBLE_REPORT_ID 0x03

static inline bool is_xbox_bt_controller(uint16_t vid, uint16_t pid)
{
    if (vid != XBOX_BT_VID)
        return false;
    switch (pid)
    {
    case XBOX_BT_ONE_S_LEGACY_PID:
    case XBOX_BT_ONE_S_PID:
    case XBOX_BT_ELITE_2_PID:
    case XBOX_BT_SERIES_PID:
    case XBOX_BT_ONE_S_BLE_PID:
    case XBOX_BT_ELITE_2_BLE_PID:
        return true;
    default:
        return false;
    }
}

struct XboxBtState
{
    bool a, b, x, y;
    bool lb, rb;
    bool back, start, guide, share;
    bool ls, rs;
    bool up, down, left, right;
    uint16_t lx = 0x8000, ly = 0x8000, rx = 0x8000, ry = 0x8000;
    uint16_t lt, rt;
    // Once a separate guide report shows up, ignore the guide bit in report 0x01
    bool has_guide_report;
};

static inline uint16_t xbox_bt_u16(const uint8_t *buf)
{
    return buf[0] | (buf[1] << 8);
}

static inline uint16_t xbox_bt_trigger(const uint8_t *buf)
{
    uint16_t val = xbox_bt_u16(buf) & 0x3FF;
    return (uint32_t)val * UINT16_MAX / 0x3FF;
}

static inline void xbox_bt_parse_report(const uint8_t *buf, uint16_t len, bool has_share, XboxBtState &out)
{
    if (len < 2)
        return;
    if (buf[0] == XBOX_BT_GUIDE_REPORT_ID)
    {
        out.has_guide_report = true;
        out.guide = buf[1] & 0x01;
        return;
    }
    if (buf[0] != XBOX_BT_INPUT_REPORT_ID || len < 16)
        return;

    out.lx = xbox_bt_u16(buf + 1);
    out.ly = xbox_bt_u16(buf + 3);
    out.rx = xbox_bt_u16(buf + 5);
    out.ry = xbox_bt_u16(buf + 7);
    out.lt = xbox_bt_trigger(buf + 9);
    out.rt = xbox_bt_trigger(buf + 11);

    uint8_t hat = buf[13];
    out.up = hat == 1 || hat == 2 || hat == 8;
    out.right = hat == 2 || hat == 3 || hat == 4;
    out.down = hat == 4 || hat == 5 || hat == 6;
    out.left = hat == 6 || hat == 7 || hat == 8;

    if (len == 16)
    {
        // Original One S firmware
        out.a = buf[14] & 0x01;
        out.b = buf[14] & 0x02;
        out.x = buf[14] & 0x04;
        out.y = buf[14] & 0x08;
        out.lb = buf[14] & 0x10;
        out.rb = buf[14] & 0x20;
        out.back = buf[14] & 0x40;
        out.start = buf[14] & 0x80;
        out.ls = buf[15] & 0x01;
        out.rs = buf[15] & 0x02;
        return;
    }

    out.a = buf[14] & 0x01;
    out.b = buf[14] & 0x02;
    out.x = buf[14] & 0x08;
    out.y = buf[14] & 0x10;
    out.lb = buf[14] & 0x40;
    out.rb = buf[14] & 0x80;
    out.back = buf[15] & 0x04;
    out.start = buf[15] & 0x08;
    if (!out.has_guide_report)
    {
        out.guide = buf[15] & 0x10;
    }
    out.ls = buf[15] & 0x20;
    out.rs = buf[15] & 0x40;
    bool extra = buf[16] & 0x01;
    if (has_share)
    {
        out.share = extra;
    }
    else
    {
        // Some firmware sends View as AC Back instead of a button
        out.back |= extra;
    }
}

static inline bool xbox_bt_tick_digital(const XboxBtState &state, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadButton_tag)
        return false;
    switch (type.mapping.gamepadButton)
    {
    case Gamepad_A:               return state.a;
    case Gamepad_B:               return state.b;
    case Gamepad_X:               return state.x;
    case Gamepad_Y:               return state.y;
    case Gamepad_LeftShoulder:    return state.lb;
    case Gamepad_RightShoulder:   return state.rb;
    case Gamepad_Back:            return state.back;
    case Gamepad_Start:           return state.start;
    case Gamepad_Guide:           return state.guide;
    case Gamepad_Capture:         return state.share;
    case Gamepad_LeftThumbClick:  return state.ls;
    case Gamepad_RightThumbClick: return state.rs;
    case Gamepad_DpadUp:          return state.up;
    case Gamepad_DpadDown:        return state.down;
    case Gamepad_DpadLeft:        return state.left;
    case Gamepad_DpadRight:       return state.right;
    default:                      return false;
    }
}

static inline uint16_t xbox_bt_tick_analog(const XboxBtState &state, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadAxis_tag)
        return 0;
    // HID axes report down as the maximum, so Y is inverted
    switch (type.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:   return state.lx;
    case Gamepad_LeftStickY:   return UINT16_MAX - state.ly;
    case Gamepad_RightStickX:  return state.rx;
    case Gamepad_RightStickY:  return UINT16_MAX - state.ry;
    case Gamepad_LeftTrigger:  return state.lt;
    case Gamepad_RightTrigger: return state.rt;
    default:                   return 0;
    }
}

// Rumble is output report 0x03: enable mask, magnitudes (0 - 100) for the left trigger, right trigger,
// strong (left) and weak (right) motors, then duration / start delay in 10ms units and loop count.
// The payload excludes the report id, as both transports send that separately.
#define XBOX_BT_RUMBLE_LEN 8
#define XBOX_BT_RUMBLE_ENABLE_WEAK 0x01
#define XBOX_BT_RUMBLE_ENABLE_STRONG 0x02
// A rumble command only lasts up to 2.55 seconds, so it gets resent while held
#define XBOX_BT_RUMBLE_REFRESH_MS 1000
#define XBOX_BT_OUTPUT_MIN_INTERVAL_MS 10

static inline void xbox_bt_build_rumble(uint8_t left, uint8_t right, uint8_t *out)
{
    out[0] = XBOX_BT_RUMBLE_ENABLE_WEAK | XBOX_BT_RUMBLE_ENABLE_STRONG;
    out[1] = 0;
    out[2] = 0;
    out[3] = left * 100 / UINT8_MAX;
    out[4] = right * 100 / UINT8_MAX;
    out[5] = 0xFF;
    out[6] = 0;
    out[7] = 0;
}

// Shared rumble bookkeeping, the transport specific hosts only do the actual send
struct XboxBtRumble
{
    uint8_t left = 0;
    uint8_t right = 0;
    bool dirty = true;
    uint32_t last_sent_ms = 0;

    void set(uint8_t l, uint8_t r)
    {
        if (l != left || r != right)
        {
            left = l;
            right = r;
            dirty = true;
        }
    }

    bool should_send(uint32_t now) const
    {
        if (now - last_sent_ms < XBOX_BT_OUTPUT_MIN_INTERVAL_MS)
            return false;
        return dirty || ((left || right) && now - last_sent_ms >= XBOX_BT_RUMBLE_REFRESH_MS);
    }

    void sent(uint32_t now)
    {
        dirty = false;
        last_sent_ms = now;
    }
};
