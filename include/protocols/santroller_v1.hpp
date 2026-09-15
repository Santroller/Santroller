#pragma once
#include <stdint.h>
#include <stdbool.h>

// Santroller 1 PC Reports (from Ardwiino-old include/reports/pc_reports.h)

typedef struct {
    uint8_t reportId;
    uint8_t x : 1;  // square
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t y : 1;  // triangle
    uint8_t leftShoulder : 1;   // l1
    uint8_t rightShoulder : 1;  // r1
    uint8_t l2 : 1;             // l2
    uint8_t r2 : 1;             // r2

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3
    uint8_t guide : 1;    // ps
    uint8_t capture : 1;  // switch capture button
    uint8_t : 2;

    uint8_t dpad; // hat (0..7, 8=neutral) or bitmask for dancepad

    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
} __attribute__((packed)) Santroller1Gamepad_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow
    uint8_t x : 1;  // square, blue

    uint8_t leftShoulder : 1;   // orange, l1
    uint8_t rightShoulder : 1;  // spPedal, r1
    uint8_t back : 1;           // select
    uint8_t start : 1;

    uint8_t guide : 1;  // ps
    uint8_t : 7;

    uint8_t dpad;

    uint8_t whammy;
    uint8_t slider;
    uint8_t tilt;
} __attribute__((packed)) Santroller1GuitarHeroGuitar_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow
    uint8_t x : 1;  // square, blue

    uint8_t leftShoulder : 1;  // orange, l1

    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;

    uint8_t soloOrange : 1;
    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t : 3;

    uint8_t dpad;

    uint8_t whammy;
    uint8_t pickup;
    uint8_t tilt;
} __attribute__((packed)) Santroller1RockBandGuitar_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow
    uint8_t x : 1;  // square, blue

    uint8_t rightShoulder : 1;  // orange, r1
    uint8_t leftShoulder : 1;   // kick, l1
    uint8_t back : 1;           // select
    uint8_t start : 1;

    uint8_t guide : 1;  // ps
    uint8_t : 7;

    uint8_t dpad;

    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t orangeVelocity;
    uint8_t kickVelocity;
} __attribute__((packed)) Santroller1GuitarHeroDrums_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t x : 1;  // square
    uint8_t y : 1;  // triangle

    uint8_t padFlag : 1;
    uint8_t cymbalFlag : 1;
    uint8_t leftShoulder : 1;   // kick 1
    uint8_t rightShoulder : 1;  // kick 2

    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t guide : 1;
    uint8_t : 5;

    uint8_t dpad;

    uint8_t greenVelocity;
    uint8_t redVelocity;
    uint8_t yellowVelocity;
    uint8_t blueVelocity;
    uint8_t greenCymbalVelocity;
    uint8_t yellowCymbalVelocity;
    uint8_t blueCymbalVelocity;
} __attribute__((packed)) Santroller1RockBandDrums_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t a : 1;  // cross, black1
    uint8_t b : 1;  // circle, black2
    uint8_t y : 1;  // triangle, black3

    uint8_t x : 1;              // square, white1
    uint8_t leftShoulder : 1;   // white2, l1
    uint8_t rightShoulder : 1;  // white3, r1
    uint8_t back : 1;           // back, heroPower
    uint8_t start : 1;          // start, pause

    uint8_t leftThumbClick : 1;  // leftThumbClick, ghtv
    uint8_t guide : 1;           // ps
    uint8_t : 6;

    uint8_t dpad;

    uint8_t whammy;
    uint8_t tilt;
} __attribute__((packed)) Santroller1GHLGuitar_Data_t;

typedef struct
{
    uint8_t reportId;

    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t x : 1;  // square
    uint8_t y : 1;  // triangle, euphoria

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t rightGreen : 1;

    uint8_t rightRed : 1;
    uint8_t rightBlue : 1;
    uint8_t leftGreen : 1;
    uint8_t leftRed : 1;

    uint8_t leftBlue : 1;
    uint8_t : 3;

    uint8_t dpad;

    uint8_t leftTableVelocity;
    uint8_t rightTableVelocity;

    uint8_t effectsKnob;
    uint8_t crossfader;
} __attribute__((packed)) Santroller1Turntable_Data_t;

