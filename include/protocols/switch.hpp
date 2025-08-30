#pragma once
#include <stdint.h>
#define SWITCH_PRO_CON_FULL_REPORT_ID 0x30
typedef struct {
    int16_t accX, accY, accZ;
    int16_t gyroX, gyroY, gyroZ;
} __attribute__((packed)) ImuData;

typedef struct {
    uint8_t connection_info : 4;
    uint8_t battery_level : 4;

    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t b : 1;
    uint8_t a : 1;

    uint8_t dummy1 : 2;
    uint8_t rightShoulder : 1;
    uint8_t rightTrigger : 1;

    uint8_t back : 1; // minus
    uint8_t start : 1; // plus
    uint8_t leftThumbClick : 1;   // l3
    uint8_t rightThumbClick : 1;  // r3

    uint8_t guide : 1; // home
    uint8_t capture : 1;
    uint8_t dummy2 : 2;

    uint8_t dpad : 4;

    uint8_t dummy3 : 2;
    uint8_t leftShoulder : 1;
    uint8_t leftTrigger : 1;

    // Bytes 6-11
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    uint8_t vibratorInput;  // What is this used for?

    // Bytes 13-48
    // Three samples of the IMU is sent in one message
    // See: https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering/blob/master/imu_sensor_notes.md
    ImuData imu[3];
} __attribute__((packed)) SwitchProGamepad_Data_t;

typedef struct {
    // Button bits
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;

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

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    uint8_t unused2[12];
    uint16_t unused3[4];

} __attribute__((packed)) SwitchGamepad_Data_t;


// Festivals default mappings suck, so its easiest to set up custom mappings for that here
typedef struct
{
    uint8_t y : 1;              // y, triangle, yellow
    uint8_t rightShoulder : 1;   // b, circle, red
    uint8_t leftShoulder : 1;  // a, cross, green
    uint8_t x : 1;              // x, square, blue

    uint8_t dpadUp : 1;    // leftShoulder, orange, l1
    uint8_t dpadDown : 1;  // rightShoulder, spPedal, r1
    uint8_t back: 1;           // back, select
    uint8_t start : 1;     // start

    uint8_t guide : 1;  // ps
    uint8_t : 2;
    uint8_t whammy : 1;
    uint8_t tilt : 1;
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    union {
        struct {
            uint8_t b : 1;          // dpadUp
            uint8_t dpadRight : 1;  // dpadDown
            uint8_t a : 1;          // dpadLeft
            uint8_t dpadLeft : 1;   // dpadRight
            uint8_t : 4;
        };
        uint8_t dpad;
    };

    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t slider;
} __attribute__((packed)) SwitchFestivalProGuitarLayer_Data_t;


typedef struct {
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;
    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t l2 : 1; // l2 rimLeft
    uint8_t r2 : 1; // r2 rimRight

    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t leftThumbClick : 1; //l3 centerLeft
    uint8_t rightThumbClick : 1; //r3 centerRight
    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t : 2;

    uint8_t dpad;
    uint8_t sticks[4]; // always 0x80
    uint8_t : 8;

} __attribute__((packed)) SwitchTaiko_Data_t;