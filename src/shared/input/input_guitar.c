#include "input_guitar.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "input_direct.h"
#include "mpu6050/mpu.h"
#include "pins/pins.h"
#include <stdbool.h>
#include <util/delay.h>
volatile bool ready = false;

int32_t z;
analog_info_t analog;
void mpu_tick(controller_t *controller) {
  if (ready) {
    ready = false;
    mympu_update();
    z = (mympu.ypr[config.axis.mpu_6050_orientation / 2] * (65535 / M_PI));
    if (config.axis.mpu_6050_orientation & 1) { z = -z; }
    if (z > 32767) { z = 65535 - z; }
    z += config.axis.tilt_sensitivity;
    z = constrain(z, 0, 32767);
    if (isnan(z)) { z = 0; }
  }
  controller->r_y = z;
}
void digital_tick(controller_t *controller) {
  controller->r_y = (!digitalRead(config.pins.r_y.pin)) * 32767;
}
void (*tick)(controller_t *controller) = NULL;

bool is_drum(void) {
  return config.main.input_type == PS3_GUITAR_HERO_DRUMS ||
         PS3_ROCK_BAND_DRUMS || WII_ROCK_BAND_DRUMS ||
         XINPUT_GUITAR_HERO_DRUMS || XINPUT_ROCK_BAND_DRUMS;
}
bool is_not_guitar(void) {
  return config.main.sub_type != PS3_GUITAR_HERO_GUITAR &&
         config.main.sub_type != PS3_ROCK_BAND_GUITAR &&
         config.main.sub_type != XINPUT_GUITAR_HERO_GUITAR &&
         config.main.sub_type != XINPUT_GUITAR_HERO_DRUMS &&
         config.main.sub_type != XINPUT_LIVE_GUITAR;
}
void guitar_init(void) {
  if (is_not_guitar()) return;
  if (config.main.tilt_type == MPU_6050) {
    mympu_open(15);
    enablePCI(config.pins.r_y.pin);
    tick = mpu_tick;
  } else if (config.main.tilt_type == DIGITAL) {
    pinMode(config.pins.r_y.pin, INPUT_PULLUP);
    tick = digital_tick;
  } else if (config.main.tilt_type == ANALOGUE &&
             config.main.input_type == WII) {
    direct_init();
    tick = direct_tick;
  }
}
int16_t r_x;
void guitar_tick(controller_t *controller) {
  r_x = controller->r_x;
  // Whammy needs to be scaled so that it is picked up
  if (r_x > 0) r_x = 0;
  r_x = r_x << 1;
  if (r_x > 0) r_x = -32767;
  controller->r_x = -r_x;
  if (tick == NULL) return;
  tick(controller);
}
ISR(PCINT0_vect) { ready = true; }
#if defined(PCINT1_vect)
ISR(PCINT1_vect) { ready = true; }
ISR(PCINT2_vect) { ready = true; }
#endif