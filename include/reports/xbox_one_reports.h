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
    uint8_t needsAck : 1;
    uint8_t internal : 1;
    uint8_t chunkStart : 1;
    uint8_t chunked : 1;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed)) GipHeader_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t unk1;
    uint8_t innerCommand;
    uint8_t innerClient : 4;
    uint8_t innerNeedsAck : 1;
    uint8_t innerInternal : 1;
    uint8_t innerChunkStart : 1;
    uint8_t innerChunked : 1;
    uint16_t bytesReceived;
    uint16_t unk2;
    uint16_t remainingBuffer;
} __attribute__((packed)) Gip_Ack_t;
typedef struct {
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view
} __attribute__((packed)) XboxOneInputHeader_Data_t;
typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t leftThumbClick : 1;
    uint8_t rightThumbClick : 1;

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
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t dpadUp : 1;    // dpadStrumUp
    uint8_t dpadDown : 1;  // dpadStrumDown
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;  // orange
    uint8_t : 1;
    uint8_t solo : 1;  // leftThumbClick
    uint8_t : 1;

    uint8_t tilt;
    uint8_t whammy;
    uint8_t pickup;

    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t : 3;

    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t : 3;

    uint8_t unknown[3];
} __attribute__((packed)) XboxOneRockBandGuitar_Data_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;  // green
    uint8_t b : 1;  // red
    uint8_t x : 1;  // blue
    uint8_t y : 1;  // yellow

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t leftShoulder : 1;   // kick1
    uint8_t rightShoulder : 1;  // kick2
    uint8_t : 1;
    uint8_t : 1;

    uint8_t yellowVelocity : 4;
    uint8_t redVelocity : 4;
    uint8_t greenVelocity : 4;
    uint8_t blueVelocity : 4;

    uint8_t blueCymbalVelocity : 4;
    uint8_t yellowCymbalVelocity : 4;
    uint8_t : 4;
    uint8_t greenCymbalVelocity : 4;
} __attribute__((packed)) XboxOneRockBandDrums_Data_t;

// This isnt actually real but by doing this we can get some sane gamepad mappings in turntable mode
typedef struct
{
    GipHeader_t Header;
    uint8_t sync : 1;
    uint8_t guide : 1;
    uint8_t start : 1;  // menu
    uint8_t back : 1;   // view

    uint8_t a : 1;
    uint8_t b : 1;
    uint8_t x : 1;
    uint8_t y : 1;

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;

    uint8_t : 4;

    uint16_t leftTrigger;
    uint16_t rightTrigger;
    
    uint16_t leftGreen : 1;
    uint16_t leftRed : 1;
    uint16_t leftBlue : 1;
    uint16_t : 13;

    uint16_t rightGreen : 1;
    uint16_t rightRed : 1;
    uint16_t rightBlue : 1;
    uint16_t : 13;

    int16_t leftTableVelocity;
    int16_t rightTableVelocity;
    int16_t effectsKnob;
    int16_t crossfader;
} __attribute__((packed)) XboxOneTurntable_Data_t;

typedef struct
{
    GipHeader_t Header;
    PS3GHLGuitar_Data_t report;
} __attribute__((packed)) XboxOneGHLGuitar_Data_t;
typedef struct
{
    XboxOneGHLGuitar_Data_t guitar;
    XboxOneGamepad_Data_t gamepad;
} __attribute__((packed)) XboxOneGHLGuitarWithGamepad_Data_t;

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
    uint8_t subCommand;  // Assumed based on the descriptor reporting a larger max length than what this uses
    uint8_t flags;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t leftMotor;
    uint8_t rightMotor;
    uint8_t duration;  // in deciseconds?
    uint8_t delay;     // in deciseconds?
    uint8_t repeat;    // in deciseconds?
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
