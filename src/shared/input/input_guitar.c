#include "input_guitar.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "mpu6050/mpu.h"
#include "pins/pins.h"
#include <stdbool.h>
#include <util/delay.h>
volatile bool ready = false;

int32_t z;
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
void analogue_tick(controller_t *controller) {
  controller->r_y =
      analogRead(config.pins.r_y.pin) + config.axis.tilt_sensitivity;
}
void (*tick)(controller_t *controller) = NULL;
void guitar_init(void) {
  if (config.main.sub_type != PS3_GUITAR_HERO_GUITAR &&
             config.main.sub_type != PS3_ROCK_BAND_GUITAR &&
             config.main.sub_type != XINPUT_GUITAR &&
             config.main.sub_type != XINPUT_GUITAR_BASS &&
             config.main.sub_type != XINPUT_LIVE_GUITAR) return;
  if (config.main.tilt_type == MPU_6050) {
    mympu_open(15);
    enablePCI(config.pins.r_y.pin);
    tick = mpu_tick;
  } else if (config.main.tilt_type == DIGITAL) {
    pinMode(config.pins.r_y.pin, INPUT_PULLUP);
    tick = digital_tick;
  } else if (config.main.tilt_type == ANALOGUE) {
    pinMode(config.pins.r_y.pin, INPUT);
    tick = analogue_tick;
  }
}
void guitar_tick(controller_t *controller) {
  if (tick == NULL) return;
  tick(controller);
  // Whammy needs to be scaled so that it is picked up
  int32_t whammy = controller->r_x * 2L;
  controller->r_x = constrain(whammy, 0, 32767);
}
ISR(PCINT0_vect) { ready = true; }
#if defined(PCINT1_vect)
ISR(PCINT1_vect) { ready = true; }
ISR(PCINT2_vect) { ready = true; }
#endif