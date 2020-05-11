#pragma once
#include "controller_defines.h"
#include <stdint.h>

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
  uint16_t all_axis[XBOX_BTN_COUNT+XBOX_AXIS_COUNT];
} controller_t;
typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
  ledstate_t leds;
} controller_a_t;