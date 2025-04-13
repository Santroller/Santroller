#pragma once
#include <stdint.h>
typedef struct {
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
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;
    uint8_t leftTrigger;   // pressure_l2
    uint8_t rightTrigger;  // pressure_r2

    int32_t gyro[3];
    int32_t accel[3];
} san_gamepad_t;

typedef struct {
    uint8_t pressureDpadUp;
    uint8_t pressureDpadRight;
    uint8_t pressureDpadLeft;
    uint8_t pressureDpadDown;
    uint8_t pressureLeftShoulder;
    uint8_t pressureRightShoulder;
    uint8_t pressureA;
    uint8_t pressureB;
    uint8_t pressureX;
    uint8_t pressureY;
} san_gamepad_pressures_t;