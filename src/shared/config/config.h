#pragma once
#pragma pack(push)
#pragma pack(1)
#include "../controller/controller.h"
#include <stdbool.h>
#include <stdint.h>
typedef struct {
  uint8_t pin;
  bool inverted;
} AnalogPin_t;

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
  AnalogPin_t lt;
  AnalogPin_t rt;
  AnalogPin_t l_x;
  AnalogPin_t l_y;
  AnalogPin_t r_x;
  AnalogPin_t r_y;
} Pins_t;

// A version of the above struct, but with everything combined into arrays
// instead of seperated.
typedef struct {
  uint8_t buttons[16];
  AnalogPin_t axis[6];
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
  uint8_t type[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
  uint8_t note[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
  uint8_t channel[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
} MidiConfig_t;

typedef struct {
  bool rfInEnabled;
  uint32_t id;
} RFConfig_t;
typedef struct {
  int16_t multiplier;
  int16_t offset;
  int16_t deadzone;
} AxisScale_t;
typedef struct {
  AxisScale_t lt;
  AxisScale_t rt;
  AxisScale_t l_x;
  AxisScale_t l_y;
  AxisScale_t r_x;
  AxisScale_t r_y;
} AxisScaleConfig_t;

typedef struct {
  uint8_t buttons;
  uint8_t strum;
  bool combinedStrum;
} DebounceConfig_t;

typedef struct {
  bool wtNeck;
  bool gh5Neck;
  bool gh5NeckBar;
  bool wiiNeck;
  bool ps2Neck;
} NeckConfig_t;

typedef struct {
  MainConfig_t main;
  Pins_t pins;
  AxisConfig_t axis;
  Keys_t keys;
  Led_t leds[XBOX_AXIS_COUNT + XBOX_BTN_COUNT];
  MidiConfig_t midi;
  RFConfig_t rf;
  uint8_t pinsSP;
  AxisScaleConfig_t axisScale;
  DebounceConfig_t debounce;
  NeckConfig_t neck; 
} Configuration_t;
#pragma pack(pop)