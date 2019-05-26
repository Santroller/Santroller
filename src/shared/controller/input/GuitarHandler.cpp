#include "GuitarHandler.h"
bool GuitarHandler::isGuitar() {
  return config.subtype == GUITAR_SUBTYPE ||
         config.subtype == GUITAR_BASS_SUBTYPE ||
         config.subtype == GUITAR_ALTERNATE_SUBTYPE;
}
void GuitarHandler::init() {
  if (config.tilt_type == MPU_6050) {
    mympu_open(15);
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
      int32_t z = (mympu.ypr[2] * (65535 / M_PI));
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
    // controller.r_x = rand() * 100;
  } else if (config.tilt_type == GRAVITY) {
    controller->r_y = IO::digitalRead(config.pins.r_y) * 32767;
  }
  //Whammy needs to be scaled so that it is picked up
  int32_t whammy = controller->r_x * 2;
  controller->r_x = constrain(whammy, 0, 32767);;
}

ISR(PCINT0_vect) { ready = true; }
ISR(PCINT1_vect) { ready = true; }
ISR(PCINT2_vect) { ready = true; }