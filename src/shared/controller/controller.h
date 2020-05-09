#pragma once
#include <stdint.h>
enum xbox_buttons_t {
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
#define XBOX_BTN_COUNT 16
#define XBOX_AXIS_COUNT 6

enum switch_buttons_t {
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
typedef struct {
  uint8_t leds[5];
  uint32_t gui;
} ledstate_t;
typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
  ledstate_t leds;
} controller_t;