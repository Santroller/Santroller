#pragma once

#include <stdint.h>

constexpr uint16_t SWITCH_ARCADE_VID = 0x0F0D;
constexpr uint16_t SWITCH_ARCADE_PID = 0x00FB;
constexpr uint16_t SWITCH_TATACON_PID = 0x00F0;

// Both HORI-compatible controllers use the same unnumbered, eight-byte HID input report.
struct SwitchArcadeReport
{
    uint8_t buttons_low;
    uint8_t buttons_high;
    uint8_t hat;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
    uint8_t vendor;
};
static_assert(sizeof(SwitchArcadeReport) == 8, "Switch arcade HID report must be eight bytes");

enum SwitchArcadeButton : uint16_t
{
    SwitchArcade_Y       = 1u << 0,
    SwitchArcade_B       = 1u << 1,
    SwitchArcade_A       = 1u << 2,
    SwitchArcade_X       = 1u << 3,
    SwitchArcade_L       = 1u << 4,
    SwitchArcade_R       = 1u << 5,
    SwitchArcade_ZL      = 1u << 6,  // Tatacon Ka Left
    SwitchArcade_ZR      = 1u << 7,  // Tatacon Ka Right
    SwitchArcade_Minus   = 1u << 8,
    SwitchArcade_Plus    = 1u << 9,
    SwitchArcade_LS      = 1u << 10, // Tatacon Don Left
    SwitchArcade_RS      = 1u << 11, // Tatacon Don Right
    SwitchArcade_Home    = 1u << 12,
    SwitchArcade_Capture = 1u << 13,
};

inline uint16_t switch_arcade_buttons(const SwitchArcadeReport &report)
{
    return uint16_t(report.buttons_low) | (uint16_t(report.buttons_high) << 8);
}

inline void switch_arcade_set_button(SwitchArcadeReport &report, uint16_t mask)
{
    report.buttons_low |= uint8_t(mask);
    report.buttons_high |= uint8_t(mask >> 8);
}

// During mapping the high nibble holds a direction bitmask (up, down, left, right).
// Convert it to the HID hat's clockwise 0..7 positions; 8 is neutral.
inline void switch_arcade_finish_hat(SwitchArcadeReport &report)
{
    const uint8_t directions = report.hat >> 4;
    const bool up = (directions & 1) && !(directions & 2);
    const bool down = (directions & 2) && !(directions & 1);
    const bool left = (directions & 4) && !(directions & 8);
    const bool right = (directions & 8) && !(directions & 4);
    report.hat = up ? (right ? 1 : left ? 7 : 0)
               : down ? (right ? 3 : left ? 5 : 4)
               : right ? 2 : left ? 6 : 8;
}
