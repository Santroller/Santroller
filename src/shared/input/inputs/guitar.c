#include "guitar.h"
#include "../pins/pins.h"
#include "config/eeprom.h"
#include "direct.h"
#include "mpu6050/inv_mpu.h"
#include "mpu6050/inv_mpu_dmp_motion_driver.h"
#include "mpu6050/mpu_math.h"
#include "util/util.h"
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>
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
#define DRUM 1
#define GUITAR 2
uint8_t types[MIDI_ROCK_BAND_DRUMS + 1] = {
    [PS3_GUITAR_HERO_DRUMS] = DRUM,      [PS3_ROCK_BAND_DRUMS] = DRUM,
    [WII_ROCK_BAND_DRUMS] = DRUM,        [XINPUT_ROCK_BAND_DRUMS] = DRUM,
    [XINPUT_GUITAR_HERO_DRUMS] = DRUM,   [MIDI_ROCK_BAND_DRUMS] = DRUM,
    [MIDI_GUITAR_HERO_DRUMS] = DRUM,     [PS3_GUITAR_HERO_GUITAR] = GUITAR,
    [PS3_ROCK_BAND_GUITAR] = GUITAR,     [WII_ROCK_BAND_GUITAR] = GUITAR,
    [XINPUT_ROCK_BAND_GUITAR] = GUITAR,  [XINPUT_GUITAR_HERO_GUITAR] = GUITAR,
    [XINPUT_LIVE_GUITAR] = GUITAR,       [MIDI_ROCK_BAND_GUITAR] = GUITAR,
    [MIDI_GUITAR_HERO_GUITAR] = GUITAR,  [MIDI_LIVE_GUITAR] = GUITAR,
    [KEYBOARD_GUITAR_HERO_DRUMS] = DRUM, [KEYBOARD_GUITAR_HERO_GUITAR] = GUITAR,
    [KEYBOARD_ROCK_BAND_DRUMS] = DRUM,   [KEYBOARD_ROCK_BAND_GUITAR] = GUITAR,
    [KEYBOARD_LIVE_GUITAR] = GUITAR};
bool isDrum(void) { return types[config.main.subType] == DRUM; }
bool isGuitar(void) { return types[config.main.subType] == GUITAR; }
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
  if (!isGuitar()) return;
  if (config.main.tiltType == MPU_6050) {
  initMPU6050(30);
  tick = tickMPUTilt;
  } else if (config.main.tiltType == DIGITAL) {
    pinMode(config.pins.r_y.pin, INPUT_PULLUP);
    tick = tickDigitalTilt;
  } else if (config.main.tiltType == ANALOGUE && config.main.inputType == WII) {
    initDirectInput();
    tick = tickDirectInput;
  }
}
int16_t r_x;
void tickGuitar(Controller_t *controller) {
  if (!isGuitar()) return;
  r_x = controller->r_x;
  // Whammy needs to be scaled so that it is picked up
  if (r_x > 0) r_x = 0;
  r_x = r_x << 1;
  if (r_x > 0) r_x = -32767;
  controller->r_x = -r_x;
  if (tick == NULL) return;
  tick(controller);
}