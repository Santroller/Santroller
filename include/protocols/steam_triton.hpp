#pragma once
#include <stdint.h>
#include <string.h>
#include "input.pb.h"
#include "protocols/motion.hpp"

// 2026 Steam Controller (Triton), see SDL's SDL_hidapi_steam_triton.c

// Input report IDs
#define TRITON_REPORT_STATE           0x42
#define TRITON_REPORT_BATTERY         0x43
#define TRITON_REPORT_STATE_BLE       0x45
#define TRITON_REPORT_WIRELESS_X      0x46
#define TRITON_REPORT_STATE_TIMESTAMP 0x47
#define TRITON_REPORT_WIRELESS        0x79

#define TRITON_WIRELESS_DISCONNECT    1
#define TRITON_WIRELESS_CONNECT       2

// Feature reports use report ID 1 followed by a {type, length, payload} message
#define TRITON_FEATURE_REPORT_ID      0x01
#define TRITON_FEATURE_REPORT_LEN     64
#define TRITON_CMD_SET_SETTINGS       0x87
#define TRITON_SETTING_LIZARD_MODE    0x09
#define TRITON_SETTING_IMU_MODE       0x30
// SDL's SETTING_GYRO_MODE_SEND_RAW_ACCEL | SETTING_GYRO_MODE_SEND_RAW_GYRO
#define TRITON_IMU_MODE_RAW           0x18

// The controller re-enables lizard mode on a watchdog, so it has to be disabled periodically
#define TRITON_LIZARD_REFRESH_MS      3000

// Feature report message (without the report ID) that turns lizard mode off and raw motion on, as
// 3 byte settings: the setting, then its value
static const uint8_t TRITON_SETTINGS_MSG[] = {
    TRITON_CMD_SET_SETTINGS, 6,
    TRITON_SETTING_LIZARD_MODE, 0x00, 0x00,
    TRITON_SETTING_IMU_MODE, TRITON_IMU_MODE_RAW, 0x00};

// State reports without the report ID are this long, for both 0x45 and 0x47
#define TRITON_STATE_PAYLOAD_LEN      45

// BLE uses the same Valve service as the original Steam Controller. Feature reports are written
// (without the report ID) to the 100F6C34 characteristic, and each input report has its own
// characteristic whose notifications carry the report without its ID.
// 100F6C7A-1735-4313-B402-38567131E5F3, report 0x45
static const uint8_t TRITON_BLE_INPUT_0x45_UUID[16] = {
    0x10, 0x0F, 0x6C, 0x7A, 0x17, 0x35, 0x43, 0x13,
    0xB4, 0x02, 0x38, 0x56, 0x71, 0x31, 0xE5, 0xF3};
// 100F6C7C-1735-4313-B402-38567131E5F3, report 0x47
static const uint8_t TRITON_BLE_INPUT_0x47_UUID[16] = {
    0x10, 0x0F, 0x6C, 0x7C, 0x17, 0x35, 0x43, 0x13,
    0xB4, 0x02, 0x38, 0x56, 0x71, 0x31, 0xE5, 0xF3};

#define TRITON_BTN_A                  0x00000001
#define TRITON_BTN_B                  0x00000002
#define TRITON_BTN_X                  0x00000004
#define TRITON_BTN_Y                  0x00000008
#define TRITON_BTN_QAM                0x00000010
#define TRITON_BTN_R3                 0x00000020
#define TRITON_BTN_VIEW               0x00000040
#define TRITON_BTN_R4                 0x00000080
#define TRITON_BTN_R5                 0x00000100
#define TRITON_BTN_R                  0x00000200
#define TRITON_BTN_DPAD_DOWN          0x00000400
#define TRITON_BTN_DPAD_RIGHT         0x00000800
#define TRITON_BTN_DPAD_LEFT          0x00001000
#define TRITON_BTN_DPAD_UP            0x00002000
#define TRITON_BTN_MENU               0x00004000
#define TRITON_BTN_L3                 0x00008000
#define TRITON_BTN_STEAM              0x00010000
#define TRITON_BTN_L4                 0x00020000
#define TRITON_BTN_L5                 0x00040000
#define TRITON_BTN_L                  0x00080000
#define TRITON_BTN_RIGHT_PAD_CLICK    0x00400000
#define TRITON_BTN_RT_CLICK           0x00800000
#define TRITON_BTN_LEFT_PAD_CLICK     0x04000000
#define TRITON_BTN_LT_CLICK           0x08000000

struct SteamTritonState
{
    uint32_t buttons = 0;
    int16_t trigger_l = 0;
    int16_t trigger_r = 0;
    int16_t left_x = 0;
    int16_t left_y = 0;
    int16_t right_x = 0;
    int16_t right_y = 0;
    int16_t accel[3] = {};
    int16_t gyro[3] = {};
    bool has_imu = false;
};

static inline bool steam_triton_is_state_report(uint8_t report_id)
{
    return report_id == TRITON_REPORT_STATE || report_id == TRITON_REPORT_STATE_BLE ||
           report_id == TRITON_REPORT_STATE_TIMESTAMP;
}

// All state report variants share the same layout up to the trackpads, after the report ID:
// [seq] [buttons u32] [trig L] [trig R] [LX] [LY] [RX] [RY] (all int16 LE)
static inline bool steam_triton_parse_state(const uint8_t *buf, uint16_t len, SteamTritonState &out)
{
    if (len < 17)
        return false;
    out.buttons = buf[1] | (buf[2] << 8) | (buf[3] << 16) | ((uint32_t)buf[4] << 24);
    out.trigger_l = (int16_t)(buf[5] | (buf[6] << 8));
    out.trigger_r = (int16_t)(buf[7] | (buf[8] << 8));
    out.left_x = (int16_t)(buf[9] | (buf[10] << 8));
    out.left_y = (int16_t)(buf[11] | (buf[12] << 8));
    out.right_x = (int16_t)(buf[13] | (buf[14] << 8));
    out.right_y = (int16_t)(buf[15] | (buf[16] << 8));
    // then the trackpads and the IMU timestamp, which change size between the report variants, but
    // the motion always ends the report
    if (len >= TRITON_STATE_PAYLOAD_LEN)
    {
        for (int i = 0; i < 3; i++)
        {
            out.accel[i] = (int16_t)(buf[33 + i * 2] | (buf[34 + i * 2] << 8));
            out.gyro[i] = (int16_t)(buf[39 + i * 2] | (buf[40 + i * 2] << 8));
        }
        out.has_imu = out.accel[0] || out.accel[1] || out.accel[2];
    }
    return true;
}

// A state report starting with its report ID
static inline bool steam_triton_parse_report(const uint8_t *buf, uint16_t len, SteamTritonState &out)
{
    if (len < 1 || !steam_triton_is_state_report(buf[0]))
        return false;
    return steam_triton_parse_state(buf + 1, len - 1, out);
}

static inline bool steam_triton_tick_digital(const SteamTritonState &s, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (s.buttons & TRITON_BTN_A) != 0;
        case Gamepad_B:               return (s.buttons & TRITON_BTN_B) != 0;
        case Gamepad_X:               return (s.buttons & TRITON_BTN_X) != 0;
        case Gamepad_Y:               return (s.buttons & TRITON_BTN_Y) != 0;
        case Gamepad_LeftShoulder:    return (s.buttons & TRITON_BTN_L) != 0;
        case Gamepad_RightShoulder:   return (s.buttons & TRITON_BTN_R) != 0;
        // SDL maps the MENU bit to back and VIEW to start
        case Gamepad_Back:            return (s.buttons & TRITON_BTN_MENU) != 0;
        case Gamepad_Start:           return (s.buttons & TRITON_BTN_VIEW) != 0;
        case Gamepad_Guide:           return (s.buttons & TRITON_BTN_STEAM) != 0;
        case Gamepad_Capture:         return (s.buttons & TRITON_BTN_QAM) != 0;
        case Gamepad_LeftThumbClick:  return (s.buttons & TRITON_BTN_L3) != 0;
        case Gamepad_RightThumbClick: return (s.buttons & TRITON_BTN_R3) != 0;
        case Gamepad_DpadUp:          return (s.buttons & TRITON_BTN_DPAD_UP) != 0;
        case Gamepad_DpadDown:        return (s.buttons & TRITON_BTN_DPAD_DOWN) != 0;
        case Gamepad_DpadLeft:        return (s.buttons & TRITON_BTN_DPAD_LEFT) != 0;
        case Gamepad_DpadRight:       return (s.buttons & TRITON_BTN_DPAD_RIGHT) != 0;
        default:                      return false;
        }
    }
    return false;
}

static inline uint16_t steam_triton_trigger(int16_t value)
{
    // 0..32767
    return value <= 0 ? 0 : (uint16_t)(value * 2 + 1);
}

static inline uint16_t steam_triton_tick_analog(const SteamTritonState &s, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:   return (uint16_t)s.left_x ^ 0x8000;
        case Gamepad_LeftStickY:   return (uint16_t)s.left_y ^ 0x8000;
        case Gamepad_RightStickX:  return (uint16_t)s.right_x ^ 0x8000;
        case Gamepad_RightStickY:  return (uint16_t)s.right_y ^ 0x8000;
        case Gamepad_LeftTrigger:  return steam_triton_trigger(s.trigger_l);
        case Gamepad_RightTrigger: return steam_triton_trigger(s.trigger_r);
        default:
            if (is_motion_axis(type.mapping.gamepadAxis))
            {
                if (!s.has_imu)
                    return MOTION_ZERO;
                float accel[3], gyro[3];
                steam_triton_imu_decode(s.accel, s.gyro, accel, gyro);
                return motion_host_axis(type.mapping.gamepadAxis, accel, gyro);
            }
            return 0;
        }
    }
    return 0;
}
