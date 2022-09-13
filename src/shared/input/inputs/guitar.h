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

#ifndef __AVR_ATmega328P__
uint8_t fivetartapbindings[] = {
    [0x19 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_Y)) >> 8,
    [0x1A - 0x19] = (_BV(XBOX_Y)) >> 8,
    [0x2C - 0x19] =
        (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X)) >> 8,
    [0x2D - 0x19] = (_BV(XBOX_A) | _BV(XBOX_Y) | _BV(XBOX_X)) >> 8,
    [0x2E - 0x19] = (_BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X)) >> 8,
    [0x2F - 0x19] = (_BV(XBOX_Y) | _BV(XBOX_X)) >> 8,
    [0x46 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_X)) >> 8,
    [0x47 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_X)) >> 8,
    [0x48 - 0x19] = (_BV(XBOX_B) | _BV(XBOX_X)) >> 8,
    [0x49 - 0x19] = (_BV(XBOX_X)) >> 8,
    [0x5F - 0x19] = (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X) |
                     _BV(XBOX_LB)) >>
                    8,
    [0x60 - 0x19] =
        (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x61 - 0x19] =
        (_BV(XBOX_A) | _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x62 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x63 - 0x19] =
        (_BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x64 - 0x19] = (_BV(XBOX_B) | _BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x65 - 0x19] = (_BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x66 - 0x19] = (_BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
    [0x78 - 0x19] =
        (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_LB)) >> 8,
    [0x79 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_LB)) >> 8,
    [0x7A - 0x19] = (_BV(XBOX_A) | _BV(XBOX_Y) | _BV(XBOX_LB)) >> 8,
    [0x7B - 0x19] = (_BV(XBOX_A) | _BV(XBOX_LB)) >> 8,
    [0x7C - 0x19] = (_BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_LB)) >> 8,
    [0x7D - 0x19] = (_BV(XBOX_B) | _BV(XBOX_LB)) >> 8,
    [0x7E - 0x19] = (_BV(XBOX_Y) | _BV(XBOX_LB)) >> 8,
    [0x7F - 0x19] = (_BV(XBOX_LB)) >> 8,
    [0x95 - 0x19] = (_BV(XBOX_A)) >> 8,
    [0xB0 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_B)) >> 8,
    [0xCD - 0x19] = (_BV(XBOX_B)) >> 8,
    [0xE5 - 0x19] = (_BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y)) >> 8,
    [0xE6 - 0x19] = (_BV(XBOX_B) | _BV(XBOX_Y)) >> 8,
};
uint8_t wttapbindings[] = {[0x17] = (_BV(XBOX_A)) >> 8,
                           [0x16] = (_BV(XBOX_A)) >> 8,
                           [0x14] = (_BV(XBOX_A) | _BV(XBOX_B)) >> 8,
                           [0x11] = (_BV(XBOX_B)) >> 8,
                           [0x12] = (_BV(XBOX_B)) >> 8,
                           [0xf] = (_BV(XBOX_B) | _BV(XBOX_Y)) >> 8,
                           [0xa] = (_BV(XBOX_Y)) >> 8,
                           [0xb] = (_BV(XBOX_Y)) >> 8,
                           [0x9] = (_BV(XBOX_X) | _BV(XBOX_Y)) >> 8,
                           [0x7] = (_BV(XBOX_X)) >> 8,
                           [0x5] = (_BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
                           [0x4] = (_BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
                           [0x3] = (_BV(XBOX_X) | _BV(XBOX_LB)) >> 8,
                           [0x0] = (_BV(XBOX_LB)) >> 8};

void tickGH5Neck(Controller_t *controller) {
  uint8_t buttons;
  twi_readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, 1, &buttons);
  // TODO: the dj turntable actually gives us bits in both directions, does the neck do the same?
  // Annoyingly, the bits for the buttons presses are reversed compared to what
  // we want.
  controller->buttons |= reverse(buttons);
}
void tickGH5NeckBar(Controller_t *controller) {
  uint8_t buttons[2];
  twi_readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, 2, buttons);
  // Annoyingly, the bits for the buttons presses are reversed compared to what
  // we want.
  controller->buttons |= reverse(buttons[0]);
  lastTap = fivetartapbindings[buttons[1] - 0x19];
  controller->buttons |= lastTap << 8;
}
void tickWTNeck(Controller_t *controller) {
  controller->buttons = 0;
  long pulse = digitalReadPulse(&wtPin, LOW, 50);
  if (pulse == digitalReadPulse(&wtPin, LOW, 50)) {
    lastTap = wttapbindings[pulse >> 1];
  }
  controller->buttons |= lastTap << 8;
}
#endif
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

#ifndef __AVR_ATmega328P__
  if (config->neck.wtNeck) {
    setUpDigital(&wtPin, config, PIN_WT_NECK, 0, false, false);
    pinMode(PIN_WT_NECK, INPUT);
    tickNeck = tickWTNeck;
  } else if (config->neck.gh5Neck) {
    tickNeck = tickGH5Neck;
  } else if (config->neck.gh5NeckBar) {
    tickNeck = tickGH5NeckBar;
  }
#endif
  scale = config->axisScale.r_y;
  tiltInverted = config->pins.r_y.inverted;
}
void tickGuitar(Controller_t *controller) {
  if (!typeIsGuitar) return;
  if (tickNeck) { tickNeck(controller); }
  Guitar_t *g = (Guitar_t *)controller;
  int32_t whammy = g->whammy;
  if (whammy > 0xFFFF) { whammy = 0xFFFF; }
  if (whammy < 0) { whammy = 0; }
  g->whammy = whammy;
  if (tick == NULL) return;
  tick(controller);
}