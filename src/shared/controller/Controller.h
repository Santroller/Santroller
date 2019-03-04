#pragma once
#include "../../config/config.h"
#include "../bootloader/Bootloader.h"
#include "../mpu6050/inv_mpu.h"
#include "../mpu6050/mpu.h"
#include "../util.h"
#include <stdint.h>
#define GREEN 0
#define RED 1
#define YELLOW 2
#define BLUE 3
#define ORANGE 4
#define UP 5
#define DOWN 6
#define LEFT 7
#define RIGHT 8
#define START 9
#define SELECT 10
#define HOME 11
#define RB 12
#define K_WHAMMY 13
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