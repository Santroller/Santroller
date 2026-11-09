#pragma once
#include <stdint.h>
#include <string.h>

#include "protocols/controller_reports.hpp"
#include "config.pb.h"

// Google Stadia controller, input report 3 (Stadia_Data_t). USB and BLE send the same report, and the
// report descriptor numbers its buttons out of order (A = 1, B = 2, X = 4, Y = 5, L1 = 7, R1 = 8,
// Options = 11, Menu = 12, Stadia = 13, L3 = 14, R3 = 15, Assistant = 17, Capture = 18), so the generic
// button-index mapping gets most of them wrong. Read the fixed layout instead.

// Output report 5: strong (low frequency, left) then weak (high frequency, right) motor, 16 bit little
// endian each, as SDL's HIDAPI Stadia driver and Linux hid-google-stadiaff send. The motors keep running
// at the last level until told otherwise, so it only needs sending on changes.
#define STADIA_RUMBLE_REPORT_ID 5
#define STADIA_RUMBLE_LEN 4
#define STADIA_OUTPUT_MIN_INTERVAL_MS 10

// Fills the report payload, without the report id
inline void stadia_build_rumble(uint8_t left, uint8_t right, uint8_t *out)
{
    uint16_t strong = left * 0x101u;
    uint16_t weak = right * 0x101u;
    out[0] = strong & 0xFF;
    out[1] = strong >> 8;
    out[2] = weak & 0xFF;
    out[3] = weak >> 8;
}

struct StadiaRumble
{
    uint8_t left = 0;
    uint8_t right = 0;
    // Send once on connect, so a controller left rumbling by a previous host stops
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
        return dirty && now - last_sent_ms >= STADIA_OUTPUT_MIN_INTERVAL_MS;
    }

    void sent(uint32_t now)
    {
        dirty = false;
        last_sent_ms = now;
    }
};

// Copies a report into out, returning false for other report ids or short reports
inline bool stadia_parse_report(const uint8_t *buf, uint16_t len, Stadia_Data_t &out)
{
    if (len < sizeof(Stadia_Data_t) || buf[0] != STADIA_REPORT_ID)
    {
        return false;
    }
    memcpy(&out, buf, sizeof(Stadia_Data_t));
    return true;
}

inline bool stadia_tick_digital(const Stadia_Data_t &data, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadButton_tag)
    {
        return false;
    }
    // Hat: 0 is up, clockwise, 8 and above is centered
    uint8_t hat = data.dpad;
    bool centered = hat >= 8;
    switch (type.mapping.gamepadButton)
    {
    case Gamepad_A:
        return data.a;
    case Gamepad_B:
        return data.b;
    case Gamepad_X:
        return data.x;
    case Gamepad_Y:
        return data.y;
    case Gamepad_LeftShoulder:
        return data.leftShoulder;
    case Gamepad_RightShoulder:
        return data.rightShoulder;
    case Gamepad_Back:
        return data.back;
    case Gamepad_Start:
        return data.start;
    case Gamepad_LeftThumbClick:
        return data.leftThumbClick;
    case Gamepad_RightThumbClick:
        return data.rightThumbClick;
    case Gamepad_Guide:
        return data.guide;
    case Gamepad_Capture:
        return data.capture;
    case Gamepad_DpadUp:
        return !centered && (hat == 7 || hat <= 1);
    case Gamepad_DpadRight:
        return !centered && hat >= 1 && hat <= 3;
    case Gamepad_DpadDown:
        return !centered && hat >= 3 && hat <= 5;
    case Gamepad_DpadLeft:
        return !centered && hat >= 5;
    default:
        return false;
    }
}

inline uint16_t stadia_tick_analog(const Stadia_Data_t &data, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadAxis_tag)
    {
        return 0;
    }
    switch (type.mapping.gamepadAxis)
    {
    case Gamepad_LeftTrigger:
        return data.leftTrigger * 0x101u;
    case Gamepad_RightTrigger:
        return data.rightTrigger * 0x101u;
    case Gamepad_LeftStickX:
        return data.leftStickX * 0x101u;
    case Gamepad_LeftStickY:
        return (UINT8_MAX - data.leftStickY) * 0x101u;
    case Gamepad_RightStickX:
        return data.rightStickX * 0x101u;
    case Gamepad_RightStickY:
        return (UINT8_MAX - data.rightStickY) * 0x101u;
    default:
        return 0;
    }
}
