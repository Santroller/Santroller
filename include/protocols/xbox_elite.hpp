#pragma once
#include <stdint.h>
#include <string.h>
#include "input.pb.h"
#include "emulation/usb/usb_devices.h"

// ---------------------------------------------------------------------------
// Xbox Elite back paddles, see SDL_hidapi_xboxone.c
//
// The paddles show up wherever the controller model, firmware and transport put them, so every
// format is normalised to P1 - P4 as bits 0 - 3. P1 / P2 are the upper / lower right paddles and
// P3 / P4 the upper / lower left ones.
//
// When the selected profile maps the paddles to other buttons, those buttons get pressed instead,
// so the paddles are dropped, as SDL does.
// ---------------------------------------------------------------------------

#define XBOX_ELITE_P1 0x01
#define XBOX_ELITE_P2 0x02
#define XBOX_ELITE_P3 0x04
#define XBOX_ELITE_P4 0x08
#define XBOX_ELITE_PADDLES 0x0F

// Length of the GIP_UNMAPPED_STATE payload from Elite Series 2 5.13+ firmware
#define XBOX_ELITE_UNMAPPED_STATE_LEN 17
// The mapped buttons, sticks and triggers at the start of a GIP input report
#define XBOX_ELITE_GIP_STATE_LEN 14

static inline bool xbox_elite_has_paddles(uint16_t vid, uint16_t pid)
{
    return vid == XBOX_VID && (pid == XBOX_ELITE_PID || pid == XBOX_ELITE_2_PID ||
                               pid == XBOX_ELITE_2_BT_PID || pid == XBOX_ELITE_2_BLE_PID);
}

// GIP input report payload (without the GIP header), wired or through the wireless adapter. Other
// lengths have no paddles, as 5.13 - 5.16 firmware sends them in GIP_UNMAPPED_STATE instead.
static inline bool xbox_elite_gip_paddles(const uint8_t *data, uint16_t len, uint8_t &paddles)
{
    switch (len)
    {
    case 29:
    {
        // Elite Series 1, the raw buttons follow the mapped ones at offset 14
        uint8_t raw = memcmp(data, data + 14, 2) == 0 ? data[28] : 0;
        paddles = ((raw & 0x02) ? XBOX_ELITE_P1 : 0) | ((raw & 0x08) ? XBOX_ELITE_P2 : 0) |
                  ((raw & 0x01) ? XBOX_ELITE_P3 : 0) | ((raw & 0x04) ? XBOX_ELITE_P4 : 0);
        return true;
    }
    case 34:
        // Elite Series 2 4.x firmware, profile in byte 15
        paddles = data[15] ? 0 : data[14] & XBOX_ELITE_PADDLES;
        return true;
    case 46:
        // Elite Series 2 5.x firmware
        paddles = data[19] ? 0 : data[18] & XBOX_ELITE_PADDLES;
        return true;
    case 47:
        // Elite Series 2 5.17+ firmware
        paddles = data[20] ? 0 : data[14] & XBOX_ELITE_PADDLES;
        return true;
    default:
        return false;
    }
}

// GIP_UNMAPPED_STATE payload. A profile that leaves the buttons alone still counts as unmapped, which
// shows as the unmapped state matching the last input report.
static inline bool xbox_elite_gip_unmapped_paddles(const uint8_t *data, uint16_t len, const uint8_t *last_input,
                                                   uint8_t &paddles)
{
    if (len != XBOX_ELITE_UNMAPPED_STATE_LEN)
        return false;
    bool mapped = data[15] && memcmp(data, last_input, XBOX_ELITE_GIP_STATE_LEN) != 0;
    paddles = mapped ? 0 : data[14] & XBOX_ELITE_PADDLES;
    return true;
}

// Elite Series 2 HID input report 0x01 over Bluetooth, starting with the report ID
static inline uint8_t xbox_elite_hid_paddles(const uint8_t *buf, uint16_t len)
{
    switch (len)
    {
    case 55:
        // Initial firmware
        return buf[35] ? 0 : buf[33] & XBOX_ELITE_PADDLES;
    case 39:
        return buf[19] ? 0 : buf[17] & XBOX_ELITE_PADDLES;
    case 20:
        // 5.13+ firmware
        return buf[17] ? 0 : buf[19] & XBOX_ELITE_PADDLES;
    default:
        return 0;
    }
}

static inline bool xbox_elite_paddle_pressed(uint8_t paddles, GamepadButtonType button)
{
    switch (button)
    {
    case Gamepad_RightPaddle1: return paddles & XBOX_ELITE_P1;
    case Gamepad_RightPaddle2: return paddles & XBOX_ELITE_P2;
    case Gamepad_LeftPaddle1:  return paddles & XBOX_ELITE_P3;
    case Gamepad_LeftPaddle2:  return paddles & XBOX_ELITE_P4;
    default:                   return false;
    }
}
