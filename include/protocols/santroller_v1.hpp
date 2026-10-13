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

typedef struct {
    uint8_t reportId;
    // same as the gamepad report, but with the face buttons in a different order
    uint8_t a : 1;  // cross
    uint8_t b : 1;  // circle
    uint8_t x : 1;  // square
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

    uint8_t dpad;

    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
} __attribute__((packed)) Santroller1StageKit_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t x : 1;  // square, blue
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t green : 1;

    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;

    uint8_t pedal : 1;
    uint8_t : 3;

    uint8_t dpad;

    uint8_t tilt;
    uint8_t lowEFret;
    uint8_t aFret;
    uint8_t dFret;
    uint8_t gFret;
    uint8_t bFret;
    uint8_t highEFret;
    uint8_t lowEFretVelocity;
    uint8_t aFretVelocity;
    uint8_t dFretVelocity;
    uint8_t gFretVelocity;
    uint8_t bFretVelocity;
    uint8_t highEFretVelocity;
} __attribute__((packed)) Santroller1ProGuitar_Data_t;

typedef struct
{
    uint8_t reportId;
    uint8_t x : 1;  // square, blue
    uint8_t a : 1;  // cross, green
    uint8_t b : 1;  // circle, red
    uint8_t y : 1;  // triangle, yellow

    uint8_t back : 1;  // select
    uint8_t start : 1;
    uint8_t guide : 1;  // ps
    uint8_t overdrive : 1;

    uint8_t pedalDigital : 1;
    uint8_t : 7;

    uint8_t dpad;

    // same bit order as the PS3 keyboard, so the pro_keys.hpp helpers work on these
    uint8_t key1;
    uint8_t key2;
    uint8_t key3;

    uint8_t pedalAnalog;
    uint8_t touchPad;
    uint8_t velocities[5];  // 7 bit velocities, bit 7 of the first byte is key 25
} __attribute__((packed)) Santroller1ProKeys_Data_t;


#ifdef __cplusplus
#include "enums.pb.h"
#include "input.pb.h"

// Santroller 1 encodes its subtype in the high byte of bcdDevice, using the old firmware's
// numbering, which drifts from SubType past StageKit.
static inline SubType santroller_v1_subtype(uint16_t version)
{
    switch ((version >> 8) & 0xFF)
    {
    case 1: return Gamepad;
    case 2: return Dancepad;
    case 3: return GuitarHeroGuitar;
    case 4: return RockBandGuitar;
    case 5: return GuitarHeroDrums;
    case 6: return RockBandDrums;
    case 7: return LiveGuitar;
    case 8: return DjHeroTurntable;
    case 9: return StageKit;
    case 16: return ProKeys;
    case 17: return ProGuitarMustang;
    case 18: return ProGuitarSquier;
    case 21: return DisneyInfinity;
    case 22: return Skylanders;
    case 23: return LegoDimensions;
    case 24: return Taiko;
    default:
        // 0 (keyboard / mouse), and types that never had a PC report, all send the gamepad report
        return Gamepad;
    }
}

// Santroller 1 report decoding (report id 1, layout depends on subtype), shared by the
// BLE and USB hosts. Implemented in src/devices/santroller_v1_decode.cpp.
bool santroller_v1_tick_digital(const uint8_t *report, SubType subtype, proto_Output &type);
uint16_t santroller_v1_tick_analog(const uint8_t *report, SubType subtype, proto_Output &type);
uint16_t santroller_v1_tick_button_pressure(const uint8_t *report, SubType subtype, proto_Output &type);
#endif
