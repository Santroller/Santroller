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