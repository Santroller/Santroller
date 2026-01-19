#pragma once
#include <stdint.h>

#define PS5_INPUT_REPORT_ID 1
typedef struct
{
    uint8_t counter : 7;
    uint8_t unpressed : 1;

    // 12 bit X, followed by 12 bit Y
    uint8_t data[3];
} __attribute__((packed)) TouchpadXY;

typedef struct 
{
    TouchpadXY p1;
    TouchpadXY p2;
} __attribute__((packed)) TouchpadData;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} __attribute__((packed)) PSSensor;
typedef struct
{
    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;

    uint8_t sequence_number; // 6

    uint8_t dpad : 4;
    uint8_t x : 1; // square
    uint8_t a : 1; // cross
    uint8_t b : 1; // circle
    uint8_t y : 1; // triangle

    uint8_t leftShoulder : 1;  // l1
    uint8_t rightShoulder : 1; // r1
    uint8_t l2 : 1;            // l2
    uint8_t r2 : 1;            // r2

    uint8_t back : 1;            // share
    uint8_t start : 1;           // options
    uint8_t leftThumbClick : 1;  // l3
    uint8_t rightThumbClick : 1; // r3

    uint8_t guide : 1;
    uint8_t touchpad : 1;
    uint8_t : 6;

    uint8_t data_11;

    uint32_t auth_seq_number;

    PSSensor gyroscope;
    PSSensor accelerometer;

    uint16_t data_28_29;

    uint16_t data_30_31_0x001a;

    TouchpadData touchpad_data;

    uint8_t data_40_55[16];

    uint8_t hash[8];

} __attribute__((packed)) PS5Dpad_Data_t;
typedef struct
{
    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;
    uint8_t rightTrigger;

    uint8_t sequence_number; // 6

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t x : 1; // square
    uint8_t a : 1; // cross
    uint8_t b : 1; // circle
    uint8_t y : 1; // triangle

    uint8_t leftShoulder : 1;  // l1
    uint8_t rightShoulder : 1; // r1
    uint8_t l2 : 1;            // l2
    uint8_t r2 : 1;            // r2

    uint8_t back : 1;            // share
    uint8_t start : 1;           // options
    uint8_t leftThumbClick : 1;  // l3
    uint8_t rightThumbClick : 1; // r3

    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t mic : 1;
    uint8_t dummy : 5; // 7-9

} __attribute__((packed)) PS5Gamepad_Data_t;

typedef struct
{

    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;

    uint8_t unused1[5];

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t x : 1; // blue
    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t y : 1; // yellow

    uint8_t leftShoulder : 1; // orange
    uint8_t : 3;

    uint8_t back : 1;  // share
    uint8_t start : 1; // options
    uint8_t solo : 1;
    uint8_t p1 : 1;

    uint8_t guide : 1;
    uint8_t capture : 1; // touchpad click
    uint8_t : 6;

    uint8_t unused2;

    uint32_t packetCounter;

    uint8_t unused3[24];

    uint8_t pickup;
    uint8_t whammy;
    uint8_t tilt;

    bool green : 1;
    bool red : 1;
    bool yellow : 1;
    bool blue : 1;
    bool orange : 1;
    bool : 1;
    bool : 1;
    bool : 1;

    bool soloGreen : 1;
    bool soloRed : 1;
    bool soloYellow : 1;
    bool soloBlue : 1;
    bool soloOrange : 1;
    bool : 1;
    bool : 1;
    bool : 1;

    uint8_t unused4[11];
    uint64_t checksum;
} __attribute__((__packed__)) PS5RockBandGuitar_Data_t;
typedef struct
{

    uint8_t report_id;
    uint8_t leftStickX;
    uint8_t leftStickY;

    uint8_t unused1[5];

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t x : 1; // blue
    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t y : 1; // yellow

    uint8_t kick1 : 1; // orange
    uint8_t kick2 : 1; // orange
    uint8_t : 2;
    uint8_t back : 1;  // share
    uint8_t start : 1; // options
    uint8_t : 2;

    uint8_t guide : 1;
    uint8_t capture : 1; // touchpad click
    uint8_t : 6;

    uint8_t unused2;

    uint32_t packetCounter;

    uint8_t unused3[24];

    uint8_t redVelocity;
    uint8_t blueVelocity;
    uint8_t yellowVelocity;
    uint8_t greenVelocity;

    uint8_t yellowCymbalVelocity;
    uint8_t blueCymbalVelocity;
    uint8_t greenCymbalVelocity;
    uint8_t unused4[9];
    uint64_t checksum;
} __attribute__((__packed__)) PS5RockBandDrums_Data_t;

typedef struct
{

    uint8_t report_id;

    uint8_t leftStickX;
    uint8_t strumBar;
    uint8_t whammy;
    uint8_t tilt;

    uint8_t unused1[2];

    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t x : 1; // blue
    uint8_t a : 1; // green
    uint8_t b : 1; // red
    uint8_t y : 1; // yellow

    uint8_t leftShoulder : 1;  // orange
    uint8_t rightShoulder : 1; // orange
    uint8_t : 2;
    uint8_t back : 1;  // share
    uint8_t start : 1; // options
    uint8_t leftThumbClick : 1;
    uint8_t p1 : 1;

    uint8_t guide : 1;
    uint8_t capture : 1; // touchpad click
    uint8_t : 6;

    uint8_t unused2;

    uint32_t packetCounter;

    uint8_t unused3[40];
    uint64_t checksum;
} __attribute__((__packed__)) PS5GHLGuitar_Data_t;