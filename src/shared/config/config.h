#pragma once
#pragma pack(push)
#pragma pack(1)
#include "../controller/controller.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
  uint8_t pin;
  uint8_t threshold;
  uint8_t type;
} PinConfig_t;

typedef struct {
  PinConfig_t up;
  PinConfig_t down;
  PinConfig_t left;
  PinConfig_t right;
  PinConfig_t start;
  PinConfig_t back;
  PinConfig_t left_stick;
  PinConfig_t right_stick;
  PinConfig_t LB;
  PinConfig_t RB;
  PinConfig_t home;
  PinConfig_t capture;
  PinConfig_t a;
  PinConfig_t b;
  PinConfig_t x;
  PinConfig_t y;
  PinConfig_t lt;
  PinConfig_t rt;
  PinConfig_t l_x;
  PinConfig_t l_y;
  PinConfig_t r_x;
  PinConfig_t r_y;
} Pins_t;

// A version of the above struct, but with everything combined into arrays
// instead of seperated.
typedef struct {
  PinConfig_t pins[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
} PinsCombined_t;

typedef struct {
  uint8_t neg;
  uint8_t pos;
} AnalogKey_t;

typedef struct {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
  uint8_t start;
  uint8_t back;
  uint8_t left_stick;
  uint8_t right_stick;
  uint8_t LB;
  uint8_t RB;
  uint8_t home;
  uint8_t capture;
  uint8_t a;
  uint8_t b;
  uint8_t x;
  uint8_t y;
  uint8_t lt;
  uint8_t rt;
  AnalogKey_t l_x;
  AnalogKey_t l_y;
  AnalogKey_t r_x;
  AnalogKey_t r_y;
} Keys_t;

typedef struct {
  uint8_t inputType;
  uint8_t subType;
  uint8_t tiltType;
  uint8_t unused;
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
  uint8_t mpu6050Orientation;
  int16_t tiltSensitivity;
} AxisConfig_t;

typedef struct {
  uint8_t type[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
  uint8_t note[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
  uint8_t channel[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
} MidiConfig_t;

typedef struct {
  MainConfig_t main;
  Pins_t pins;
  AxisConfig_t axis;
  Keys_t keys;
  Led_t leds[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
  MidiConfig_t midi;
} Configuration_t;

#pragma pack(pop)