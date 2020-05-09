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
} controller_t;