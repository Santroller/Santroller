#pragma once
#include "../io/bootloader/Bootloader.h"
#include "../util.h"
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
#define XBOX_A 11
#define XBOX_B 12
#define XBOX_X 13
#define XBOX_Y 14

#define XBOX_BTN_COUNT 14

#ifdef __cplusplus
class Controller {
public:
  uint16_t buttons;
  uint8_t lt;
  uint8_t rt;
  int l_x;
  int l_y;
  int r_x;
  int r_y;
};
#endif