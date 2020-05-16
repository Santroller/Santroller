#pragma once
#include "controller_defines.h"
#include <stdint.h>

typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
  uint32_t leds[XBOX_BTN_COUNT+XBOX_AXIS_COUNT];
  uint8_t drum_axis[8];
} controller_t;

typedef struct {
  uint16_t buttons;
  uint8_t triggers[2];
  int16_t sticks[4];
  uint32_t leds[XBOX_BTN_COUNT+XBOX_AXIS_COUNT];
  uint8_t drum_axis[8];
} controller_a_t;