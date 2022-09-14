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
void tickDJ(Controller_t *controller) {
  if (!typeIsDJ) return;
  uint8_t data[3];
  if (twi_readFromPointer(DJLEFT_ADDR, DJ_BUTTONS_PTR, sizeof(data), data)) {
    if (data[1]) {
      controller->buttons |= _BV(XBOX_LB);
      controller->buttons |= data[0] << 8;
    } else {
      controller->buttons &= ~_BV(XBOX_LB);
    }
    controller->l_x = ((int8_t)data[2]) << 5;
  } else {
    controller->buttons &= ~_BV(XBOX_LB);
  }
  if (twi_readFromPointer(DJRIGHT_ADDR, DJ_BUTTONS_PTR, sizeof(data), data)) {
    if (data[1]) {
      controller->buttons |= _BV(XBOX_RB);
      controller->buttons |= data[0] << 8;
    } else {
      controller->buttons &= ~_BV(XBOX_RB);
    }
    controller->l_y = ((int8_t)data[2]) << 5;
  } else {
    controller->buttons &= ~_BV(XBOX_RB);
  }
}
