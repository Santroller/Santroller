#pragma once
#include <stdint.h>

#define PS3_STICK_CENTER 0x80
#define PS3_ACCEL_CENTER 0x0200
#define PS3_REPORT_BUFFER_SIZE 48
#define GUITAR_ONE_G 40

typedef struct {
    uint8_t header;

    uint8_t back : 1;  // select
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t start : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;

    uint8_t l2 : 1;             // l2
    uint8_t r2 : 1;             // r2
    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t y : 1;  // triangle
    uint8_t b : 1;  // circle
    uint8_t a : 1;  // cross
    uint8_t x : 1;  // square

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftStickX;
    uint8_t leftStickY;

    // Pressure axes for buttons
    // Neutral state is 0x00, max is 0xFF
    uint8_t pressureDpadRight;
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadUp;
    uint8_t pressureDpadDown;
    uint8_t pressureTriangle;
    uint8_t pressureCircle;
    uint8_t pressureCross;
    uint8_t pressureSquare;
    uint8_t pressureL1;
    uint8_t pressureR1;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2
} __attribute__((packed)) PS2Gamepad_Data_t;

typedef struct
{
    uint8_t header;

    uint8_t back : 1;  // select
    uint8_t : 1;
    uint8_t : 1;
    uint8_t start : 1;
    uint8_t dpadUp : 1;
    uint8_t : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;

    uint8_t tilt : 1;
    uint8_t green : 1;  // cross, green
    uint8_t pedal: 1;
    uint8_t : 1;
    uint8_t yellow : 1;  // triangle, yellow
    uint8_t red : 1;  // circle, red
    uint8_t blue : 1;  // square, blue
    uint8_t orange : 1;   // orange, l1
    uint8_t dpad;
    uint8_t unused1;
    uint8_t slider;
    uint8_t whammy;
    uint8_t leftStickX;
    uint8_t leftStickY;
} __attribute__((packed)) PS2GuitarHeroGuitar_Data_t;

