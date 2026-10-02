#pragma once
#include <stdint.h>

// DanceCon2040 Spice2x: ID 1, little-endian controller and panel fields.
constexpr uint16_t SPICE2X_VID = 0x1209;
constexpr uint16_t SPICE2X_PID = 0x39dd;
constexpr uint8_t SPICE2X_INPUT_ID = 1;
constexpr uint8_t SPICE2X_PANEL_LED_ID = 2;
constexpr uint8_t SPICE2X_STATUS_LED_ID = 3;

struct Spice2xInputReport {
    uint8_t id;
    uint16_t controller;
    uint16_t pad;
} __attribute__((packed));
static_assert(sizeof(Spice2xInputReport) == 5, "Spice2x input is five bytes");

enum Spice2xPad : uint16_t {
    SpiceUpLeft = 1 << 2, SpiceUp = 1 << 3, SpiceUpRight = 1 << 4,
    SpiceLeft = 1 << 5, SpiceCenter = 1 << 6, SpiceRight = 1 << 7,
    SpiceDownLeft = 1 << 8, SpiceDown = 1 << 9, SpiceDownRight = 1 << 10
};
