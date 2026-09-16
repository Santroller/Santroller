#pragma once
#include <stdint.h>
#include <string.h>
#include "input.pb.h"

// ---------------------------------------------------------------------------
// Steam Controller Commands & Registers
// ---------------------------------------------------------------------------
#define STEAM_CMD_SET_MAPPINGS         0x80
#define STEAM_CMD_CLEAR_MAPPINGS       0x81
#define STEAM_CMD_GET_MAPPINGS         0x82
#define STEAM_CMD_WRITE_REGISTER       0x87
#define STEAM_CMD_CLEAR_REGISTER       0x88
#define STEAM_CMD_READ_REGISTER        0x89
#define STEAM_CMD_SET_MODE             0x8D
#define STEAM_CMD_HAPTIC_RUMBLE        0xEB

#define STEAM_REG_LPAD_MODE            0x07
#define STEAM_REG_RPAD_MODE            0x08
#define STEAM_REG_RPAD_MARGIN          0x18
#define STEAM_REG_LED                  0x2D
#define STEAM_REG_GYRO_MODE            0x30

// Command buffers to clear digital mappings and disable lizard mode
static const uint8_t STEAM_CMD_CLEAR_MAPPINGS_BUF[] = {
    0xC0, STEAM_CMD_CLEAR_MAPPINGS, 0x01
};

static const uint8_t STEAM_CMD_DISABLE_LIZARD_BUF[] = {
    0xC0, STEAM_CMD_WRITE_REGISTER, 0x0F,
    STEAM_REG_GYRO_MODE,   0x00, 0x00, // Disable gyro
    STEAM_REG_LPAD_MODE,   0x07, 0x00, // Disable trackpad cursor emulation
    STEAM_REG_RPAD_MODE,   0x07, 0x00, // Disable mouse emulation
    STEAM_REG_RPAD_MARGIN, 0x00, 0x00, // Margin 0
    STEAM_REG_LED,         0x64, 0x00  // Center LED brightness
};

// BLE UUIDs for Valve Steam Controller
// Service:        100F6C32-1735-4313-B402-38567131E5F3
// Characteristic: 100F6C34-1735-4313-B402-38567131E5F3
static const uint8_t STEAM_BLE_SERVICE_UUID[16] = {
    0x10, 0x0F, 0x6C, 0x32, 0x17, 0x35, 0x43, 0x13,
    0xB4, 0x02, 0x38, 0x56, 0x71, 0x31, 0xE5, 0xF3
};

static const uint8_t STEAM_BLE_CHAR_REPORT_UUID[16] = {
    0x10, 0x0F, 0x6C, 0x34, 0x17, 0x35, 0x43, 0x13,
    0xB4, 0x02, 0x38, 0x56, 0x71, 0x31, 0xE5, 0xF3
};

// State representation
struct SteamControllerState {
    uint32_t buttons = 0;
    int16_t  stick_x = 0;
    int16_t  stick_y = 0;
    int16_t  pad_x = 0;
    int16_t  pad_y = 0;
    uint8_t  trigger_l = 0;
    uint8_t  trigger_r = 0;
};

// Button bits in 32-bit state:
#define STEAM_BTN_RT_CLICK      0x00000001
#define STEAM_BTN_LT_CLICK      0x00000002
#define STEAM_BTN_RB            0x00000004
#define STEAM_BTN_LB            0x00000008
#define STEAM_BTN_Y             0x00000010
#define STEAM_BTN_B             0x00000020
#define STEAM_BTN_X             0x00000040
#define STEAM_BTN_A             0x00000080
#define STEAM_BTN_DPAD_UP       0x00000100
#define STEAM_BTN_DPAD_RIGHT    0x00000200
#define STEAM_BTN_DPAD_LEFT     0x00000400
#define STEAM_BTN_DPAD_DOWN     0x00000800
#define STEAM_BTN_SELECT        0x00001000
#define STEAM_BTN_GUIDE         0x00002000
#define STEAM_BTN_START         0x00004000
#define STEAM_BTN_GRIP_L        0x00008000
#define STEAM_BTN_GRIP_R        0x00010000
#define STEAM_BTN_STICK_CLICK   0x00400000
#define STEAM_BTN_PAD_CLICK     0x00800000

static inline bool steam_tick_digital(const SteamControllerState &s, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (s.buttons & STEAM_BTN_A) != 0;
        case Gamepad_B:               return (s.buttons & STEAM_BTN_B) != 0;
        case Gamepad_X:               return (s.buttons & STEAM_BTN_X) != 0;
        case Gamepad_Y:               return (s.buttons & STEAM_BTN_Y) != 0;
        case Gamepad_LeftShoulder:    return (s.buttons & (STEAM_BTN_LB | STEAM_BTN_GRIP_L)) != 0;
        case Gamepad_RightShoulder:   return (s.buttons & (STEAM_BTN_RB | STEAM_BTN_GRIP_R)) != 0;
        case Gamepad_Back:            return (s.buttons & STEAM_BTN_SELECT) != 0;
        case Gamepad_Start:           return (s.buttons & STEAM_BTN_START) != 0;
        case Gamepad_LeftThumbClick:  return (s.buttons & STEAM_BTN_STICK_CLICK) != 0;
        case Gamepad_RightThumbClick: return (s.buttons & STEAM_BTN_PAD_CLICK) != 0;
        case Gamepad_Guide:           return (s.buttons & STEAM_BTN_GUIDE) != 0;
        case Gamepad_DpadUp:          return (s.buttons & STEAM_BTN_DPAD_UP) != 0;
        case Gamepad_DpadDown:        return (s.buttons & STEAM_BTN_DPAD_DOWN) != 0;
        case Gamepad_DpadLeft:        return (s.buttons & STEAM_BTN_DPAD_LEFT) != 0;
        case Gamepad_DpadRight:       return (s.buttons & STEAM_BTN_DPAD_RIGHT) != 0;
        default:                      return false;
        }
    }
    return false;
}

static inline uint16_t steam_tick_analog(const SteamControllerState &s, proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:   return (uint16_t)(s.stick_x + 32768);
        case Gamepad_LeftStickY:   return (uint16_t)(-s.stick_y + 32768);
        case Gamepad_RightStickX:  return (uint16_t)(s.pad_x + 32768);
        case Gamepad_RightStickY:  return (uint16_t)(-s.pad_y + 32768);
        case Gamepad_LeftTrigger:  return (uint16_t)s.trigger_l * 0x101;
        case Gamepad_RightTrigger: return (uint16_t)s.trigger_r * 0x101;
        default:                   return 0;
        }
    }
    return 0;
}

// Parse BLE report (20 bytes)
static inline bool steam_parse_ble_report(const uint8_t *report, uint16_t len, SteamControllerState &out)
{
    if (len < 10) return false;
    if (report[0] != 0x03 || report[1] != 0xC0) return false;
    if ((report[2] & 0x0F) != 0x04) return false;

    uint16_t flags = (report[2] & 0xF0) | (report[3] << 8);
    int idx = 4;

    if (flags & 0x0010) // BUTTONS
    {
        if (idx + 3 <= len)
        {
            out.buttons = report[idx] | (report[idx + 1] << 8) | (report[idx + 2] << 16);
            idx += 3;
        }
    }
    if (flags & 0x0020) // TRIGGERS
    {
        if (idx + 2 <= len)
        {
            out.trigger_l = report[idx];
            out.trigger_r = report[idx + 1];
            idx += 2;
        }
    }
    if (flags & 0x0080) // THUMBSTICK
    {
        if (idx + 4 <= len)
        {
            out.stick_x = (int16_t)(report[idx] | (report[idx + 1] << 8));
            out.stick_y = (int16_t)(report[idx + 2] | (report[idx + 3] << 8));
            idx += 4;
        }
    }
    if (flags & 0x0100) // LEFT PAD
    {
        idx += 4;
    }
    if (flags & 0x0200) // RIGHT PAD
    {
        if (idx + 4 <= len)
        {
            out.pad_x = (int16_t)(report[idx] | (report[idx + 1] << 8));
            out.pad_y = (int16_t)(report[idx + 2] | (report[idx + 3] << 8));
        }
    }
    return true;
}

// Parse USB report (64 bytes)
static inline bool steam_parse_usb_report(const uint8_t *buf, uint16_t len, SteamControllerState &out)
{
    if (len < 14) return false;

    // Wired USB reports from Valve Steam Controller (0x01 0x00 0x01 header)
    if (buf[0] == 0x01 && buf[1] == 0x00 && buf[2] == 0x01)
    {
        // Buttons 3 bytes at offset 8..10
        out.buttons = buf[8] | (buf[9] << 8) | (buf[10] << 16);
        out.trigger_l = buf[11];
        out.trigger_r = buf[12];
        out.stick_x = (int16_t)(buf[16] | (buf[17] << 8));
        out.stick_y = (int16_t)(buf[18] | (buf[19] << 8));
        out.pad_x = (int16_t)(buf[20] | (buf[21] << 8));
        out.pad_y = (int16_t)(buf[22] | (buf[23] << 8));
        return true;
    }

    // Wireless dongle / BLE encapsulation (starts with 0x03 0xC0)
    if (buf[0] == 0x03 && buf[1] == 0xC0)
    {
        return steam_parse_ble_report(buf, len, out);
    }

    return false;
}
