#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
#ifndef NUM_DIGITAL_PINS
#define NUM_DIGITAL_PINS 70
#define NUM_ANALOG_INPUTS 20
#define PORTS 6
#define PINS_PER_PORT 7
#endif
typedef struct {
    uint8_t pin;
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
    uint8_t type;
    uint8_t note;
    uint8_t channel;
} Midi_t;
typedef struct {
    uint8_t binding;
    Led_t led;
    Midi_t midi;
    AxisScale_t scale;
    bool digital;
} Binding_t;


typedef struct {
    Binding_t digital[PORTS * 7];
} Bindings_t;
uint16_t test = sizeof(Bindings_t);


typedef struct {
    uint8_t inputType;
    uint8_t subType;
    uint8_t tiltType;
    uint8_t pollRate;
    uint8_t fretLEDMode;
    bool mapLeftJoystickToDPad;
    bool mapStartSelectToHome;
    bool mapNunchukAccelToRightJoy;
    uint32_t signature;
    uint32_t version;
} MainConfig_t;

typedef struct {
    uint8_t triggerThreshold;
    uint8_t joyThreshold;
    uint8_t drumThreshold;
    uint8_t mpu6050Orientation;
    int16_t tiltSensitivity;
} AxisConfig_t;

typedef struct {
    bool rfInEnabled;
    uint32_t id;
} RFConfig_t;

typedef struct {
    uint8_t buttons;
    uint8_t strum;
    bool combinedStrum;
} DebounceConfig_t;

typedef struct {
    MainConfig_t main;
    AxisConfig_t axis;
    RFConfig_t rf;
    uint8_t pinsSP;
    DebounceConfig_t debounce;
} Configuration_t;