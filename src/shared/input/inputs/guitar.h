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
#include <stdio.h>
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12
// Constants used by the mpu 6050
#define FSR 2000
//#define GYRO_SENS       ( 131.0f * 250.f / (float)FSR )
#define GYRO_SENS 16.375f
#define QUAT_SENS 1073741824.f // 2^30
// We want to scale values up by 128, as we are doing fixed point calculations.
#define QUAT_SENS_FP 8388608L // 2^23
union u_quat q;
int16_t mpuTilt;
AnalogInfo_t analog;
volatile bool ready = false;
bool gh5Neck = false;
bool gh5Tap = false;
uint8_t mpuOrientation;
Pin_t tiltPin;
bool tiltInverted;
AxisScale_t scale;
Pin_t wtPin;
uint8_t lastTap;
void tickMPUTilt(Controller_t *controller) {
  static short sensors;
  static unsigned char fifoCount;
  dmp_read_fifo(NULL, NULL, q._l, NULL, &sensors, &fifoCount);
  q._f.w = q._l[0] >> 23;
  q._f.x = q._l[1] >> 23;
  q._f.y = q._l[2] >> 23;
  q._f.z = q._l[3] >> 23;

  quaternionToEuler(&q._f, &mpuTilt, mpuOrientation);
  mpuTilt = tiltInverted ? -mpuTilt : mpuTilt;
  analogueData[XBOX_TILT] = mpuTilt;
  int32_t val = mpuTilt;
  val -= scale.offset;
  val *= scale.multiplier;
  val /= 1024;
  val += INT16_MIN;
  if (val > INT16_MAX) val = INT16_MAX;
  if (val < INT16_MIN) val = INT16_MIN;
  // if (val < scale.deadzone) { val = INT16_MIN; }
  controller->r_y = val;
}
void tickDigitalTilt(Controller_t *controller) {
  controller->r_y = digitalReadPin(&tiltPin) ? 32767 : 0;
}
void (*tick)(Controller_t *controller) = NULL;
void (*tickNeck)(Controller_t *controller) = NULL;
// Would it be worth only doing this check once for speed?
void initMPU6050(unsigned int rate) {
  sei();
  mpu_init(NULL);
  mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  mpu_set_gyro_fsr(FSR);
  mpu_set_accel_fsr(2);
  mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  dmp_load_motion_driver_firmware();
  dmp_set_fifo_rate(rate);
  mpu_set_dmp_state(1);
  dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT);
}

void initGuitar(Configuration_t *config) {
  if (!typeIsGuitar) return;
  if (config->main.tiltType == MPU_6050) {
    mpuOrientation = config->axis.mpu6050Orientation;
    initMPU6050(30);
    tick = tickMPUTilt;
  } else if (config->main.tiltType == DIGITAL) {
    setUpDigital(&tiltPin, config, config->pins.r_y.pin, 0, false, false);
    pinMode(tiltPin.pin, INPUT_PULLUP);
    tick = tickDigitalTilt;
  }

  gh5Neck = config->neck.gh5Neck || config->neck.gh5NeckBar;

  scale = config->axisScale.r_y;
  tiltInverted = config->pins.r_y.inverted;
}
void tickGuitar(Controller_t *controller) {
  if (!typeIsGuitar) return;
  if (gh5Neck) {
    uint8_t data[2];
    if (twi_readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, sizeof(data),
                            data)) {
      controller->buttons &= ~(_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB));
      controller->buttons |= data[0] << 8;
    } 
  }
  Guitar_t *g = (Guitar_t *)controller;
  int32_t whammy = g->whammy;
  if (whammy > 0xFFFF) { whammy = 0xFFFF; }
  if (whammy < 0) { whammy = 0; }
  g->whammy = whammy;
  if (tick == NULL) return;
  tick(controller);
}