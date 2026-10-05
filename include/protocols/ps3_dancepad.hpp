#pragma once
#include <cstddef>
#include <cstdint>

constexpr uint16_t PS3_DANCEPAD_VID = 0x1CCF;
constexpr uint16_t PS3_DANCEPAD_PID = 0x1010;
constexpr uint8_t PS3_DANCEPAD_NEUTRAL_HAT = 8;

// No report ID: 13 buttons + 3 padding bits, hat + padding, four axes,
// twelve vendor bytes, then four little-endian vendor words.
struct PS3DancepadReport {
    uint8_t buttons1;
    uint8_t buttons2;
    uint8_t hat;
    uint8_t x, y, z, rz;
    uint8_t vendor_right, vendor_left, vendor_up, vendor_down;
    uint8_t vendor_north, vendor_east, vendor_south, vendor_west;
    uint8_t vendor_28, vendor_29, vendor_2a, vendor_2b;
    uint16_t vendor_2c, vendor_2d, vendor_2e, vendor_2f;
} __attribute__((packed));
static_assert(sizeof(PS3DancepadReport) == 27, "DanceCon2040 PS3 input is 27 bytes");
static_assert(offsetof(PS3DancepadReport, vendor_right) == 7, "Vendor bytes start at byte 7");
static_assert(offsetof(PS3DancepadReport, vendor_2c) == 19, "Vendor words start at byte 19");

struct PS3InstrumentOutput {
    uint8_t output_type;
    uint8_t data_length;
    uint8_t player_led;
    uint8_t unknown2[5];
} __attribute__((packed));
static_assert(sizeof(PS3InstrumentOutput) == 8, "PS3 instrument output is 8 bytes");
static_assert(offsetof(PS3InstrumentOutput, player_led) == 2, "Player LED mask is output byte 2");

constexpr bool ps3_dancepad_direction(uint8_t hat, uint8_t direction) {
    return hat < 8 && (direction == 0 ? (hat == 7 || hat == 0 || hat == 1) :
                       direction == 1 ? (hat == 1 || hat == 2 || hat == 3) :
                       direction == 2 ? (hat == 3 || hat == 4 || hat == 5) :
                                        (hat == 5 || hat == 6 || hat == 7));
}
