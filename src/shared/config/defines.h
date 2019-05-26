#pragma once
#define NONE 0

//Tilt detection
#define MPU_6050 1
#define GRAVITY 2

//Input types
#define WII 1
#define DIRECT 2

//Output types
#define XINPUT 1
#define KEYBOARD 2
#define GAMEPAD 3

//Joystick type for a guitar
#define DPAD 1
#define JOY 2

//Subtypes for XINPUT
#define GAMEPAD_SUBTYPE 1
#define WHEEL_SUBTYPE 2
#define ARCADE_STICK_SUBTYPE 3
#define FLIGHT_STICK_SUBTYPE 4
#define DANCE_PAD_SUBTYPE 5
#define GUITAR_SUBTYPE 6
#define GUITAR_ALTERNATE_SUBTYPE 7
#define DRUM_SUBTYPE 8
#define GUITAR_BASS_SUBTYPE 11
#define ARCADE_PAD_SUBTYPE 19

#define INVALID_PIN 0xFF

//Orientations for gyros
#define Z_P 0
#define Z_N 1
#define Y_P 2
#define Y_N 3
#define X_P 4
#define X_N 5