#pragma once
#include <stdint.h>

// Reports for the original Nintendo controller ports (GameCube / N64 over joybus, SNES / NES
// over the latch + clock shift register). Bitfields are in the order the bits go out on the wire.

#define GC_STICK_CENTER 0x80

// GameCube poll response (mode 3), https://www.int03.co.uk/crema/hardware/gamecube/gc-control.html
typedef struct
{
    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;
    uint8_t start : 1;
    uint8_t : 3;

    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadUp : 1;
    uint8_t z : 1;
    uint8_t r : 1;
    uint8_t l : 1;
    uint8_t alwaysOne : 1;

    // 0x00 is left / down, 0xFF is right / up
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t cStickX;
    uint8_t cStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
} __attribute__((packed)) GameCubeGamepad_Data_t;

// Real sticks only reach about this far from center, so games are tuned for it
#define N64_STICK_RANGE 85

// N64 poll response
typedef struct
{
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadUp : 1;
    uint8_t start : 1;
    uint8_t z : 1;
    uint8_t b : 1;
    uint8_t a : 1;

    uint8_t cRight : 1;
    uint8_t cLeft : 1;
    uint8_t cDown : 1;
    uint8_t cUp : 1;
    uint8_t r : 1;
    uint8_t l : 1;
    uint8_t : 1;
    uint8_t reset : 1;

    // signed, positive is right / up
    int8_t stickX;
    int8_t stickY;
} __attribute__((packed)) N64Gamepad_Data_t;

// SNES pad state, bit 0 is the first bit the console clocks in. 1 = pressed, the wire is active low.
typedef struct
{
    uint16_t b : 1;
    uint16_t y : 1;
    uint16_t select : 1;
    uint16_t start : 1;
    uint16_t dpadUp : 1;
    uint16_t dpadDown : 1;
    uint16_t dpadLeft : 1;
    uint16_t dpadRight : 1;
    uint16_t a : 1;
    uint16_t x : 1;
    uint16_t l : 1;
    uint16_t r : 1;
    uint16_t : 4;
} __attribute__((packed)) SNESGamepad_Data_t;

// NES pad state, same wire format as SNES but only 8 bits
typedef struct
{
    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t select : 1;
    uint8_t start : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
} __attribute__((packed)) NESGamepad_Data_t;
