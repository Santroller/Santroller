#include "input_guitar.h"
#include "../config/eeprom.h"
#include "mpu6050/mpu.h"
#include "pins/pins.h"
#include <util/delay.h>
#include <stdbool.h>
#include "../util.h"
volatile bool ready = false;
bool isXboxGuitar(void) {
  return config.sub_type == XINPUT_GUITAR_SUBTYPE ||
         config.sub_type == XINPUT_GUITAR_ALTERNATE_SUBTYPE ||
         config.sub_type == XINPUT_GUITAR_BASS_SUBTYPE;
}
bool isPS3Guitar(void) {
  return config.sub_type == PS3_GUITAR_GH_SUBTYPE ||
         config.sub_type == PS3_GUITAR_RB_SUBTYPE;
}
bool isGuitar(void) { return isXboxGuitar() || isPS3Guitar(); }
void guitar_init(void) {
  if (!isGuitar()) return;
  if (config.tilt_type == MPU_6050) {
    _delay_ms(1000);
    while (mympu_open(15) != 0) {}
    enablePCI(config.pins.mpu_6050_interrupt);
  } else if (config.tilt_type == GRAVITY) {
    pinMode(config.pins.r_y, INPUT_PULLUP);
  } else if (config.tilt_type == GRAVITY) {
    pinMode(config.pins.r_y, INPUT);
  }
}

void guitar_tick(controller_t *controller) {
  if (!isGuitar()) return;
  if (config.tilt_type == MPU_6050) {
    if (ready) {
      ready = false;
      mympu_update();
      int32_t z = (mympu.ypr[config.mpu_6050_orientation / 2] * (65535 / M_PI));
      if (config.mpu_6050_orientation & 1) { z = -z; }
      if (z > 32767) { z = 65535 - z; }
      //Make this into a sensitivity option.
      z = pow(z, 1.05f);
      z = constrain(z, 0, 32767);
      if (isnan(z)) { z = 0; }
      controller->r_y = z;
    }
  } else if (config.tilt_type == GRAVITY) {
    controller->r_y = digitalRead(config.pins.r_y) * 32767;
  } else if (config.tilt_type == ANALOGUE) {
    controller->r_y = analogRead(config.pins.r_y);
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