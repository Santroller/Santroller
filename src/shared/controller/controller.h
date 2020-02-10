#pragma once
#include <stdint.h>

#define XBOX_DPAD_UP 0
#define XBOX_DPAD_DOWN 1
#define XBOX_DPAD_LEFT 2
#define XBOX_DPAD_RIGHT 3
#define XBOX_START 4
#define XBOX_BACK 5
#define XBOX_LEFT_STICK 6
#define XBOX_RIGHT_STICK 7

#define XBOX_LB 8
#define XBOX_RB 9
#define XBOX_HOME 10
#define XBOX_UNUSED 11
#define XBOX_A 12
#define XBOX_B 13
#define XBOX_X 14
#define XBOX_Y 15
#define XBOX_BTN_COUNT 16
#define SWITCH_Y 0
#define SWITCH_B 1
#define SWITCH_A 2
#define SWITCH_X 3
#define SWITCH_L 4
#define SWITCH_R 5
#define SWITCH_ZL 6
#define SWITCH_ZR 7
#define SWITCH_SELECT 8
#define SWITCH_START 9
#define SWITCH_LEFT_STICK 10
#define SWITCH_RIGHT_STICK 11
#define SWITCH_HOME 12
#define SWITCH_CAPTURE 13
typedef struct {
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int16_t l_x;
  int16_t l_y;
  int16_t r_x;
  int16_t r_y;
} controller_t;