#pragma once
#include <Arduino.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} Led_t;
typedef struct {
    int8_t multiplier;
    int8_t offset;
    int8_t deadzone;
} AxisScale_t;
typedef struct {
    uint8_t note : 7;
    bool isCC : 1;
    uint8_t channel : 4;
} Midi_t;
typedef struct {
    AxisScale_t scale;
    bool mapToDigital;
    uint8_t trigger;
    uint8_t channel;
} AnalogData_t;
typedef struct {
    uint8_t binding;
    BindingType_t type;
    bool pulldown : 1;
    bool pullup : 1;
    uint8_t led_index;
    Led_t led_on;
    Led_t led_off;
    Midi_t midi;
    uint8_t milliDeBounce;
    uint8_t analogID;
} Binding_t;

typedef struct {
    Binding_t bindings[NUM_DIGITAL_PINS];
    AnalogData_t analog[NUM_ANALOG_INPUTS];
    bool mapLeftJoystickToDPad;
    bool mapStartSelectToHome;
    bool mapNunchukAccelToRightJoy;
} Bindings_t;

typedef struct {
    uint8_t inputType;
    uint8_t subType;
    uint8_t tiltType;
    uint8_t pollRate;
    uint8_t fretLEDMode;
    uint32_t signature;
} MainConfig_t;

typedef struct {
    uint8_t mpu6050Orientation;
    int16_t tiltSensitivity;
} AxisConfig_t;

typedef struct {
    bool rfInEnabled;
    uint32_t id;
} RFConfig_t;

typedef struct {
    MainConfig_t main;
    AxisConfig_t axis;
    RFConfig_t rf;
} Configuration_t;
#define DEVICE_TYPE GUITAR_HERO_GUITAR
#if DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == ROCK_BAND_GUITAR || DEVICE_TYPE == GUITAR_HERO_LIVE_GUITAR
#define DEVICE_TYPE_IS_GUITAR true
#define DEVICE_TYPE_IS_DRUM false
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS || DEVICE_TYPE == ROCK_BAND_DRUMS
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM true
#else
#define DEVICE_TYPE_IS_GUITAR false
#define DEVICE_TYPE_IS_DRUM false
#endif

#if DEVICE_TYPE == GAMEPAD
#define SUB_TYPE XINPUT_GAMEPAD
#elif DEVICE_TYPE == WHEEL
#define SUB_TYPE XINPUT_WHEEL
#elif DEVICE_TYPE == ARCADE_STICK
#define SUB_TYPE XINPUT_ARCADE_STICK
#elif DEVICE_TYPE == GUITAR_HERO_GUITAR || DEVICE_TYPE == ROCK_BAND_GUITAR
#define SUB_TYPE XINPUT_GUITAR_ALTERNATE
#elif DEVICE_TYPE == GUITAR_HERO_LIVE_GUITAR
#define SUB_TYPE XINPUT_LIVE_GUITAR
#elif DEVICE_TYPE == GUITAR_HERO_DRUMS || DEVICE_TYPE == ROCK_BAND_DRUMS
#define SUB_TYPE XINPUT_DRUMS
#elif DEVICE_TYPE == FLIGHT_STICK
#define SUB_TYPE XINPUT_FLIGHT_STICK
#elif DEVICE_TYPE == DANCE_PAD
#define SUB_TYPE XINPUT_DANCE_PAD
#elif DEVICE_TYPE == ARCADE_PAD
#define SUB_TYPE XINPUT_ARCADE_PAD
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
#define SUB_TYPE XINPUT_TURNTABLE
#endif

// ConsoleType can change due to console detection
extern uint8_t consoleType;
#ifdef __cplusplus
}
#endif
