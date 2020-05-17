#pragma once
#include <stdint.h>

#define XBOX_BTN_COUNT 16
#define XBOX_AXIS_COUNT 6

// Standard controller structure. axis' and triggers are seperated for easy modification
typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
  uint32_t leds[XBOX_BTN_COUNT + XBOX_AXIS_COUNT];
  uint8_t drumVelocity[8];
} Controller_t;

// Combined controller structure. Axis' and triggers are combined into arrays to allow for easy modification by direct
typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
  uint32_t leds[XBOX_BTN_COUNT + XBOX_AXIS_COUNT];
  uint8_t drumVelocity[8];
} ControllerCombined_t;

// Buttons used by all controllers
enum ControllerButtons {
  XBOX_DPAD_UP,
  XBOX_DPAD_DOWN,
  XBOX_DPAD_LEFT,
  XBOX_DPAD_RIGHT,
  XBOX_START,
  XBOX_BACK,
  XBOX_LEFT_STICK,
  XBOX_RIGHT_STICK,

  XBOX_LB,
  XBOX_RB,
  XBOX_HOME,
  XBOX_UNUSED,
  XBOX_A,
  XBOX_B,
  XBOX_X,
  XBOX_Y,
};

// Buttons used for mapping to a switch or PS3 controller
enum SwitchButtons {
  SWITCH_Y,
  SWITCH_B,
  SWITCH_A,
  SWITCH_X,
  SWITCH_L,
  SWITCH_R,
  SWITCH_ZL,
  SWITCH_ZR,
  SWITCH_SELECT,
  SWITCH_START,
  SWITCH_LEFT_STICK,
  SWITCH_RIGHT_STICK,
  SWITCH_HOME,
  SWITCH_CAPTURE,
};