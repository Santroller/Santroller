#include "GuitarHandler.h"
#include <stdlib.h>
#include <util/delay.h>
bool GuitarHandler::isGuitar() {
  return config.subtype == GUITAR_SUBTYPE ||
         config.subtype == GUITAR_BASS_SUBTYPE ||
         config.subtype == GUITAR_ALTERNATE_SUBTYPE;
}
void GuitarHandler::init() {
  if (config.tilt_type == MPU_6050) {
    _delay_ms(1000);
    while (mympu_open(15) != 0) {

    }
    EN_PCI(MPU_6050_INTERRUPT_PIN);
  } else if (config.tilt_type == GRAVITY) {
    IO::pinMode(config.pins.r_y, INPUT_PULLUP);
  }
}

volatile bool ready = false;
void GuitarHandler::handle(Controller *controller) {
  if (config.tilt_type == MPU_6050) {
    if (ready) {
      ready = false;
      mympu_update();
      int32_t z = (mympu.ypr[config.mpu_6050_orientation / 2] * (65535 / M_PI));
      if (config.mpu_6050_orientation & 1) {
        z = -z;
      }
      if (z > 32767) {
        z = 65535 - z;
      }
      z = pow(z, 1.1f);
      z = constrain(z, 0, 32767);
      if (isnan(z)) {
        z = 0;
      }
      controller->r_y = z;
    }
  } else if (config.tilt_type == GRAVITY) {
    controller->r_y = IO::digitalRead(config.pins.r_y) * 32767;
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