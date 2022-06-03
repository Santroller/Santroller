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
uint16_t lastTap;
void tickMPUTilt(Controller_t *controller) {
  static short sensors;
  static unsigned char fifoCount;
  dmp_read_fifo(NULL, NULL, q._l, NULL, &sensors, &fifoCount);
  if (sensors == INV_WXYZ_QUAT) {
    q._f.w = q._l[0] >> 23;
    q._f.x = q._l[1] >> 23;
    q._f.y = q._l[2] >> 23;
    q._f.z = q._l[3] >> 23;

    quaternionToEuler(&q._f, &mpuTilt, mpuOrientation);
    mpuTilt = tiltInverted ? -mpuTilt : mpuTilt;
  }
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
  controller->r_y = digitalReadPin(tiltPin) ? 32767 : 0;
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
uint16_t fivetartapbindings[] = {[0x95] = _BV(XBOX_A),
                                 [0xCD] = _BV(XBOX_B),
                                 [0x1A] = _BV(XBOX_Y),
                                 [0x49] = _BV(XBOX_X),
                                 [0x7F] = _BV(XBOX_LB),
                                 [0xB0] = _BV(XBOX_A) | _BV(XBOX_B),
                                 [0x19] = _BV(XBOX_A) | _BV(XBOX_Y),
                                 [0x47] = _BV(XBOX_A) | _BV(XBOX_X),
                                 [0x7B] = _BV(XBOX_A) | _BV(XBOX_LB),
                                 [0xE6] = _BV(XBOX_B) | _BV(XBOX_Y),
                                 [0x48] = _BV(XBOX_B) | _BV(XBOX_X),
                                 [0x7D] = _BV(XBOX_B) | _BV(XBOX_LB),
                                 [0x2F] = _BV(XBOX_Y) | _BV(XBOX_X),
                                 [0x7E] = _BV(XBOX_Y) | _BV(XBOX_LB),
                                 [0x66] = _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x65] = _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x64] = _BV(XBOX_B) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x7C] = _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_LB),
                                 [0x2E] = _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X),
                                 [0x62] = _BV(XBOX_A) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x7A] = _BV(XBOX_A) | _BV(XBOX_Y) | _BV(XBOX_LB),
                                 [0x2D] = _BV(XBOX_A) | _BV(XBOX_Y) | _BV(XBOX_X),
                                 [0x79] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_LB),
                                 [0x46] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_X),
                                 [0xE5] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y),
                                 [0x63] = _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x61] = _BV(XBOX_A) | _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x60] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0x78] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_LB),
                                 [0x2C] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X),
                                 [0x5F] = _BV(XBOX_A) | _BV(XBOX_B) | _BV(XBOX_Y) | _BV(XBOX_X) | _BV(XBOX_LB),
                                 [0xFF] = 0};
uint16_t wttapbindings[] = {[0x2C] = _BV(XBOX_A),
                            [0x2D] = _BV(XBOX_A),
                            [0x2E] = _BV(XBOX_A),
                            [0x28] = _BV(XBOX_A) | _BV(XBOX_B),
                            [0x22] = _BV(XBOX_B),
                            [0x23] = _BV(XBOX_B),
                            [0x24] = _BV(XBOX_B),
                            [0x1E] = _BV(XBOX_B) | _BV(XBOX_X),
                            [0x13] = _BV(XBOX_Y),
                            [0x14] = _BV(XBOX_Y),
                            [0x15] = _BV(XBOX_Y),
                            [0x16] = _BV(XBOX_Y),
                            [0x12] = _BV(XBOX_X) | _BV(XBOX_Y),
                            [0xE] = _BV(XBOX_X),
                            [0xA] = _BV(XBOX_X) | _BV(XBOX_LB),
                            [0x9] = _BV(XBOX_X) | _BV(XBOX_LB),
                            [0x8] = _BV(XBOX_X) | _BV(XBOX_LB),
                            [0x7] = _BV(XBOX_X) | _BV(XBOX_LB),
                            [0x00] = _BV(XBOX_LB),
                            [0x01] = _BV(XBOX_LB)};

void tickGH5Neck(Controller_t *controller) {
  uint8_t buttons;
  twi_readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, 1, &buttons);
  // Annoyingly, the bits for the buttons presses are reversed compared to what we want.
  controller->buttons |= reverse(buttons);
}
void tickGH5NeckBar(Controller_t *controller) {
  uint8_t buttons[2];
  twi_readFromPointer(GH5NECK_ADDR, GH5NECK_BUTTONS_PTR, 2, buttons);
  // Annoyingly, the bits for the buttons presses are reversed compared to what we want.
  controller->buttons |= reverse(buttons[0]);
  lastTap = wttapbindings[buttons[1]];
  controller->buttons |= lastTap;
}
void tickWTNeck(Controller_t *controller) {
  long pulse = digitalReadPulse(wtPin, LOW, 0);
  if (pulse == digitalReadPulse(wtPin, LOW, 0)) {
    lastTap = wttapbindings[pulse];
  }
  controller->buttons |= lastTap;
}

void initGuitar(Configuration_t *config) {
  if (!typeIsGuitar) return;
  if (config->main.tiltType == MPU_6050) {
    mpuOrientation = config->axis.mpu6050Orientation;
    initMPU6050(30);
    tick = tickMPUTilt;
  } else if (config->main.tiltType == DIGITAL) {
    tiltPin = setUpDigital(config, config->pins.r_y.pin, 0, false, false);
    pinMode(tiltPin.pin, INPUT_PULLUP);
    tick = tickDigitalTilt;
  }
  if (config->neck.wtNeck) {
    wtPin = setUpDigital(config, PIN_WT_NECK, 0, false, false);
    tickNeck = tickWTNeck;
  } else if (config->neck.gh5Neck) {
    tickNeck = tickGH5Neck;
  } else if (config->neck.gh5NeckBar) {
    tickNeck = tickGH5NeckBar;
  }
  pinMode(wtPin.pin, INPUT);
  scale = config->axisScale.r_y;
  tiltInverted = config->pins.r_y.inverted;
}
void tickGuitar(Controller_t *controller) {
  if (!typeIsGuitar) return;
  if (tickNeck) {
    tickWTNeck(controller);
  }
  Guitar_t *g = (Guitar_t *)controller;
  int32_t whammy = g->whammy;
  if (whammy > 0xFFFF) { whammy = 0xFFFF; }
  if (whammy < 0) { whammy = 0; }
  g->whammy = whammy;
  if (tick == NULL) return;
  tick(controller);
}