#pragma once
#include "controller/controller.h"
#include "direct.h"
#include "eeprom/eeprom.h"
#include "guitar.h"
#include "i2c/i2c.h"
#include "mpu6050/inv_mpu.h"
#include "mpu6050/inv_mpu_dmp_motion_driver.h"
#include "mpu6050/mpu_math.h"
#include "pins/pins.h"
#include "timer/timer.h"
#include "util/util.h"
#include <stdbool.h>
#include <stdlib.h>
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
typedef struct {
  uint8_t buttons_flipped;
  uint8_t buttons;
  int16_t turntable;
} __attribute__((packed)) dj_t;
void tickDJ(Controller_t *controller) {
  dj_t dj;
  if (twi_readFromPointer(DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(dj_t),
                          (uint8_t *)&dj)) {
    if (dj.buttons) {
      controller->buttons |= _BV(XBOX_LB);
      controller->buttons |= dj.buttons_flipped << 8;
    } else {
      controller->buttons &= ~_BV(XBOX_LB);
    }
    controller->l_x = dj.turntable << 3;
  }
  if (twi_readFromPointer(DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(dj_t),
                          (uint8_t *)&dj)) {
    if (dj.buttons) {
      controller->buttons |= _BV(XBOX_RB);
      controller->buttons |= dj.buttons_flipped << 8;
    } else {
      controller->buttons &= ~_BV(XBOX_RB);
    }
    controller->l_y = dj.turntable << 3;
  }
}
