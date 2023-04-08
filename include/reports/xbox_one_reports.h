#include <stdint.h>

#include "reports/ps3_reports.h"
#define GIP_POWER_MODE_DEVICE_CONFIG 0x05
#define GIP_DEVICE_DESCRIPTOR 0x04
#define GIP_VIRTUAL_KEYCODE 0x07
#define GIP_CMD_RUMBLE 0x09
#define GIP_INPUT_REPORT 0x20
#define GHL_HID_REPORT 0x21
#define GHL_HID_OUTPUT 0x22
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
    bool guide : 1;
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
    bool guide : 1;
    bool start : 1;  // menu
    bool back : 1;   // view

    bool a : 1; // green
    bool b : 1; // red
    bool x : 1; // blue
    bool y : 1; // yellow

    bool dpadUp : 1; // dpadStrumUp
    bool dpadDown : 1; // dpadStrumDown
    bool dpadLeft : 1;
    bool dpadRight : 1;

    bool leftShoulder : 1; // orange
    bool : 1;
    bool solo : 1; // leftThumbClick
    bool : 1;

    uint8_t tilt;
    uint8_t whammy;
    uint8_t pickup;

    bool green : 1;
    bool red : 1;
    bool yellow : 1;
    bool blue : 1;
    bool orange : 1;
    uint8_t : 3;

    bool soloGreen : 1;
    bool soloRed : 1;
    bool soloYellow : 1;
    bool soloBlue : 1;
    bool soloOrange : 1;
    uint8_t : 3;

    uint8_t unknown[3];
} __attribute__((packed)) XboxOneRockBandGuitar_Data_t;

typedef struct
{
    GipHeader_t Header;
    bool sync : 1;
    bool guide : 1;
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

    uint8_t yellowVelocity : 4;
    uint8_t redVelocity : 4;
    uint8_t greenVelocity : 4;
    uint8_t blueVelocity : 4;

    uint8_t blueCymbalVelocity : 4;
    uint8_t yellowCymbalVelocity : 4;
    uint8_t : 4;
    uint8_t greenCymbalVelocity : 4;
} __attribute__((packed)) XboxOneRockBandDrums_Data_t;

typedef struct
{
    GipHeader_t Header;
    PS3GHLGuitar_Data_t report;
} __attribute__((packed)) XboxOneGHLGuitar_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sub_command;
    uint8_t data[7];
} __attribute__((packed)) XboxOneGHLGuitar_Output_t;

typedef struct
{
    GipHeader_t Header;
    bool pressed : 1;
    uint8_t : 7;
    uint8_t keycode;
} __attribute__((packed)) GipKeystroke_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t subcommand;
} __attribute__((packed)) GipPowerMode_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t data[];
} __attribute__((packed)) GipPacket_t;

typedef struct {
    GipHeader_t Header;
    uint8_t subCommand; // Assumed based on the descriptor reporting a larger max length than what this uses
    uint8_t flags;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t leftMotor;
    uint8_t rightMotor;
    uint8_t duration; // in deciseconds?
    uint8_t delay; // in deciseconds?
    uint8_t repeat; // in deciseconds?
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
