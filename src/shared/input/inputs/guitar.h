#pragma once
#include "controller/controller.h"
#include "controller/guitar_includes.h"
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
#define GH5NECK_ADDR 0x0D
#define GH5NECK_OK_PTR 0x11
#define GH5NECK_BUTTONS_PTR 0x12
// Extended GH5 slider with full multi-touch
#define GH5NECK_SLIDER_NEW_PTR 0x15
// Older style slider with WT-type detection, adjacent frets only
#define GH5NECK_SLIDER_OLD_PTR 0x16
// Constants used by the mpu 6050
#define FSR 2000
//#define GYRO_SENS       ( 131.0f * 250.f / (float)FSR )
#define GYRO_SENS 16.375f
#define QUAT_SENS 1073741824.f // 2^30
// We want to scale values up by 128, as we are doing fixed point calculations.
#define QUAT_SENS_FP 8388608.f // 2^23
union u_quat q;
int16_t z;
AnalogInfo_t analog;
volatile bool ready = false;
void tickMPUTilt(Controller_t *controller) {
  static short sensors;
  static unsigned char fifoCount;
  dmp_read_fifo(NULL, NULL, q._l, NULL, &sensors, &fifoCount);
  if (sensors == INV_WXYZ_QUAT) {
    config.axis.mpu6050Orientation = NEGATIVE_Y;
    q._f.w = (float)q._l[0] / QUAT_SENS_FP;
    q._f.x = (float)q._l[1] / QUAT_SENS_FP;
    q._f.y = (float)q._l[2] / QUAT_SENS_FP;
    q._f.z = (float)q._l[3] / QUAT_SENS_FP;

    quaternionToEuler(&q._f, &z, config.axis.mpu6050Orientation);
    z += config.axis.tiltSensitivity;
  }
  controller->r_y = z;
}
void tickDigitalTilt(Controller_t *controller) {
  controller->r_y = (!digitalRead(config.pins.r_y.pin)) * 32767;
}
void (*tick)(Controller_t *controller) = NULL;
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
void initGuitar(void) {
  if (!isGuitar(config.main.subType)) return;
  if (config.main.tiltType == MPU_6050) {
    initMPU6050(30);
    tick = tickMPUTilt;
  } else if (config.main.tiltType == DIGITAL) {
    pinMode(config.pins.r_y.pin, INPUT_PULLUP);
    tick = tickDigitalTilt;
  }
}
int16_t r_x;
void tickGuitar(Controller_t *controller) {
  if (!isGuitar(config.main.subType)) return;
  if (controller->r_x < 0x7FF) {
    controller->r_x = 0;
  } else if (controller->r_x > 0x3FFF) {
    controller->r_x = 0x7FFF;
  } else {
    controller->r_x = (controller->r_x << 1);
  }
  if (tick == NULL) return;
  tick(controller);
}

// TODO: this is all we need for grabbing data from a gh5 neck. We should test
// this, do we actually need to run it at 100khz, or does our i2c implementation
// work better somehow
void tickGH5Neck(Controller_t *controller) {
  uint8_t ok;
  twi_readFromPointer(GH5NECK_ADDR, GH5NECK_OK_PTR, 1, &ok);
  if (ok) {
    uint8_t buttons;
    twi_readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, 1, &buttons);
    uint8_t slider;
    twi_readFromPointer(GH5NECK_ADDR, GH5NECK_SLIDER_NEW_PTR, 1, &slider);
  }
}