#include <stdint.h>

#include "reports/controller_reports.h"
#define GIP_POWER_MODE_DEVICE_CONFIG 0x05
#define GIP_ARRIVAL 0x03
#define GIP_VIRTUAL_KEYCODE 0x07
#define GIP_CMD_RUMBLE 0x09
#define GIP_INPUT_REPORT 0x20
#define GHL_HID_REPORT 0x21
#define GIP_VKEY_LEFT_WIN 0x5b
typedef struct
{
    uint8_t command;
    uint8_t client : 4;
    bool needsAck : 1;
    bool internal : 1;
    bool chunkStart : 1;
    bool chunked : 1;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed)) GipHeader_t;

typedef struct
{
    GipHeader_t Header;
    bool sync : 1;
    bool : 1;
    bool start : 1;  // menu
    bool back : 1;   // view

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool leftShoulder : 1;
    bool rightShoulder : 1;
    bool leftThumbClick : 1;
    bool rightThumbClick : 1;

    uint16_t leftTrigger;
    uint16_t rightTrigger;

    int16_t leftStickX;
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;
} __attribute__((packed)) XboxOneGamepad_Data_t;

typedef struct
{
    GipHeader_t Header;
    bool sync : 1;
    bool : 1;
    bool start : 1;  // menu
    bool back : 1;   // view

    bool a : 1; // green
    bool b : 1; // red
    bool x : 1; // blue
    bool y : 1; // yellow

    bool dpadStrumUp : 1;
    bool dpadStrumDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool leftShoulder : 1; // orange
    bool : 1;
    bool solo : 1; // leftThumbClick
    bool : 1;

    uint8_t tilt;
    uint8_t whammy;
    uint8_t pickup;

    bool upperGreen : 1;
    bool upperRed : 1;
    bool upperYellow : 1;
    bool upperBlue : 1;
    bool upperOrange : 1;
    bool : 3;

    bool lowerGreen : 1;
    bool lowerRed : 1;
    bool lowerYellow : 1;
    bool lowerBlue : 1;
    bool lowerOrange : 1;
    bool : 3;

    uint8_t unknown[3];
} __attribute__((packed)) XboxOneRockBandGuitar_Data_t;

typedef struct
{
    GipHeader_t Header;
    bool sync : 1;
    bool : 1;
    bool start : 1;  // menu
    bool back : 1;   // view

    bool a : 1; // green
    bool b : 1; // red
    bool x : 1; // blue
    bool y : 1; // yellow

    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool leftShoulder : 1; // kick1
    bool rightShoulder : 1; // kick2
    bool : 1;
    bool : 1;

    uint8_t yellowPad : 4;
    uint8_t redPad : 4;
    uint8_t greenPad : 4;
    uint8_t bluePad : 4;

    uint8_t blueCymbal : 4;
    uint8_t yellowCymbal : 4;
    uint8_t : 4;
    uint8_t greenCymbal : 4;
} __attribute__((packed)) XboxOneRockBandDrums_Data_t;

typedef struct
{
    GipHeader_t Header;
    bool sync : 1;
    bool : 1;
    bool start : 1;  // menu
    bool back : 1;   // view

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool leftShoulder : 1;
    bool rightShoulder : 1;
    bool leftThumbClick : 1;
    bool rightThumbClick : 1;

    uint16_t leftTrigger;
    bool : 10;
    bool leftBlue : 1;
    bool leftRed : 1;
    bool leftGreen : 1;
    bool rightBlue : 1;
    bool rightRed : 1;
    bool rightGreen : 1;

    int16_t effectsKnob;
    int16_t crossfader;
    
    int16_t leftTableVelocity;
    int16_t rightTableVelocity;
} __attribute__((packed)) XboxOneTurntable_Data_t;

typedef struct
{
    GipHeader_t Header;
    PS3GHLGuitar_Data_t report;
} __attribute__((packed)) XboxOneGHLGuitar_Data_t;

typedef struct
{
    bool pressed : 1;
    uint8_t : 7;
    uint8_t keycode;
} __attribute__((packed)) GipKeystroke_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t subcommand;
} __attribute__((packed)) GipPowerMode_t;

typedef struct {
    GipHeader_t Header;
    uint8_t unknown;
    uint8_t motors;
    uint8_t left_trigger;
    uint8_t right_trigger;
    uint8_t left;
    uint8_t right;
    uint8_t duration;
    uint8_t delay;
    uint8_t repeat;
} __attribute__((packed)) GipRumble_t;

#define GIP_HEADER(packet, cmd, isInternal, seq) \
    packet->Header.command = cmd;                \
    packet->Header.internal = isInternal;        \
    packet->Header.sequence = seq;               \
    packet->Header.client = 0;                   \
    packet->Header.needsAck = 0;                 \
    packet->Header.chunkStart = 0;               \
    packet->Header.chunked = 0;                  \
    packet->Header.length = sizeof(*packet) - sizeof(GipHeader_t);