#pragma once
#include <stdint.h>
#include <string.h>
#include "input.pb.h"
#include "protocols/motion.hpp"

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

    // raw IMU samples, zero until motion is turned on
    int16_t accel[3] = {};
    int16_t gyro[3] = {};
    bool has_imu = false;
    // from flash, in rad/s, read over USB
    float gyro_bias[3] = {};
};

// The input report, as the controller sends it over BLE. Over USB it is the same after a leading
// report ID. See SDL_hidapi_switch2.c and github.com/trevlars/switch2-controllers-linux.
#define SWITCH2_REPORT_STICKS_LEN 16
#define SWITCH2_REPORT_MOTION_LEN 0x3C

static inline bool switch2_parse_report(const uint8_t *buf, uint16_t len, Switch2ControllerState &out)
{
    if (len < SWITCH2_REPORT_STICKS_LEN) return false;

    // Report format:
    // Offset 0..3: Timestamp
    // Offset 4..7: Buttons
    // Offset 10..12: Left stick (12-bit packed)
    // Offset 13..15: Right stick (12-bit packed)
    // Offset 0x30..0x3B: Acceleration then gyro, x / y / z each
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
    out.left_stick_x  = buf[10] | ((buf[11] & 0x0F) << 8);
    out.left_stick_y  = ((buf[11] >> 4) & 0x0F) | (buf[12] << 4);
    out.right_stick_x = buf[13] | ((buf[14] & 0x0F) << 8);
    out.right_stick_y = ((buf[14] >> 4) & 0x0F) | (buf[15] << 4);

    if (len >= SWITCH2_REPORT_MOTION_LEN)
    {
        for (int i = 0; i < 3; i++)
        {
            out.accel[i] = (int16_t)(buf[0x30 + i * 2] | (buf[0x31 + i * 2] << 8));
            out.gyro[i] = (int16_t)(buf[0x36 + i * 2] | (buf[0x37 + i * 2] << 8));
        }
        out.has_imu = out.accel[0] || out.accel[1] || out.accel[2];
    }

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
        default:
            if (is_motion_axis(type.mapping.gamepadAxis))
            {
                if (!s.has_imu)
                    return MOTION_ZERO;
                float accel[3], gyro[3];
                switch2_imu_decode(s.accel, s.gyro, s.gyro_bias, accel, gyro);
                return motion_host_axis(type.mapping.gamepadAxis, accel, gyro);
            }
            return 0;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// USB commands
// ---------------------------------------------------------------------------
// Over USB the controller takes commands on the bulk endpoints of its second, vendor specific interface,
// each answered by a reply. A command is its ID, 0x91, 0x00 for USB, a subcommand, 0x00, the length of
// what follows the 8 byte header, then 0x00 0x00. The sequence is SDL's.

// Reads 0x40 bytes of flash at the address in bytes 12 - 15, which come back from byte 0x10 of the reply
#define SWITCH2_FLASH_READ_LEN 0x40
#define SWITCH2_FLASH_REPLY_LEN 0x50
#define SWITCH2_FLASH_GYRO_CALIBRATION 0x13040
static const uint8_t SWITCH2_READ_GYRO_CALIBRATION[] = {
    0x02, 0x91, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x40, 0x30, 0x01, 0x00};

// Feature bit 0x04 is motion, which this leaves on
#define SWITCH2_FEATURE_MOTION 0x04
static const uint8_t SWITCH2_USB_INIT_UNKNOWN_1[] = {0x07, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_FEATURE_MASK[] = {0x0c, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_UNKNOWN_2[] = {0x11, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_RUMBLE_DATA[] = {
    0x0a, 0x91, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00,
    0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0x35, 0x00, 0x46, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_FEATURE_ENABLE[] = {0x0c, 0x91, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_UNKNOWN_3[] = {0x01, 0x91, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_ENABLE_RUMBLE[] = {0x01, 0x91, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_GRIP_BUTTONS[] = {0x08, 0x91, 0x00, 0x02, 0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_REPORT_FORMAT[] = {0x03, 0x91, 0x00, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};
static const uint8_t SWITCH2_USB_INIT_START_OUTPUT[] = {
    0x03, 0x91, 0x00, 0x0d, 0x00, 0x08, 0x00, 0x00,
    0x01, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct Switch2Command
{
    const uint8_t *data;
    uint8_t len;
};
#define SWITCH2_COMMAND(c) {c, sizeof(c)}
static const Switch2Command SWITCH2_USB_INIT[] = {
    SWITCH2_COMMAND(SWITCH2_READ_GYRO_CALIBRATION),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_UNKNOWN_1),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_FEATURE_MASK),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_UNKNOWN_2),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_RUMBLE_DATA),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_FEATURE_ENABLE),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_UNKNOWN_3),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_ENABLE_RUMBLE),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_GRIP_BUTTONS),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_REPORT_FORMAT),
    SWITCH2_COMMAND(SWITCH2_USB_INIT_START_OUTPUT),
};

// The gyro bias from the reply to SWITCH2_READ_GYRO_CALIBRATION, as three floats in rad/s from byte 4
// of the flash block
static inline bool switch2_parse_gyro_calibration(const uint8_t *reply, uint16_t len, float bias[3])
{
    if (len < 0x10 + 16)
        return false;
    for (int i = 0; i < 3; i++)
    {
        float value;
        memcpy(&value, reply + 0x10 + 4 + i * 4, sizeof(value));
        // erased flash, or a reply to something else
        if (!(value > -1.0f && value < 1.0f))
            return false;
        bias[i] = value;
    }
    return true;
}
