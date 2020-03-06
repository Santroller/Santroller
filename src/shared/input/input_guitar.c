#include "input_guitar.h"
#include "../config/eeprom.h"
#include "mpu6050/mpu.h"
#include "pins/pins.h"
#include <util/delay.h>
#include <stdbool.h>
#include "../util.h"
volatile bool ready = false;
bool isXboxGuitar(void) {
  return config.main.sub_type == XINPUT_GUITAR ||
         config.main.sub_type == XINPUT_GUITAR_BASS;
}
bool isPS3Guitar(void) {
  return config.main.sub_type == PS3_GUITAR_HERO_GUITAR ||
         config.main.sub_type == PS3_ROCK_BAND_GUITAR;
}
bool isGuitar(void) { return isXboxGuitar() || isPS3Guitar(); }
void guitar_init(void) {
  if (!isGuitar()) return;
  if (config.main.tilt_type == MPU_6050) {
    mympu_open(15);
    enablePCI(config.pins.r_y.pin);
  } else if (config.main.tilt_type == DIGITAL) {
    pinMode(config.pins.r_y.pin, INPUT_PULLUP);
  } else if (config.main.tilt_type == ANALOGUE) {
    pinMode(config.pins.r_y.pin, INPUT);
  }
}
int32_t z, t0,t1,t2;
void guitar_tick(controller_t *controller) {
  if (!isGuitar()) return;
  if (config.main.tilt_type == MPU_6050) {
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
  } else if (config.main.tilt_type == DIGITAL) {
    controller->r_y = (!digitalRead(config.pins.r_y.pin)) * 32767;
  } else if (config.main.tilt_type == ANALOGUE) {
    controller->r_y = analogRead(config.pins.r_y.pin) + config.axis.tilt_sensitivity;
  }
  // Whammy needs to be scaled so that it is picked up
  int32_t whammy = controller->r_x * 2L;
  controller->r_x = constrain(whammy, 0, 32767);
}
ISR(PCINT0_vect) { ready = true; }
#if defined(PCINT1_vect)
ISR(PCINT1_vect) { ready = true; }
ISR(PCINT2_vect) { ready = true; }
#endif