#pragma once
#define NONE 0

//Tilt detection
#define MPU_6050 1
#define GRAVITY 2

//Input types
#define WII 1
#define DIRECT 2

//Output types
#define X_INPUT 1
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
#define POSITIVE_Z 0
#define NEGATIVE_Z 1
#define POSITIVE_Y 2
#define NEGATIVE_Y 3
#define POSITIVE_X 4
#define NEGATIVE_X 5

//Firmware types
#define GUITAR_FIRMWARE 0xfea123