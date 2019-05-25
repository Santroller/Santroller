#include "InputHandler.h"
#include "../../io/bootloader/Bootloader.h"
void InputHandler::process() {
  if (input != NULL) {
    input->read_controller(&controller);
    if (config.map_joy_to_dpad) {
      CHECK_JOY(l_x, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT);
      CHECK_JOY(l_y, XBOX_DPAD_DOWN, XBOX_DPAD_UP);
    }
    processTilt();
  }
}
void InputHandler::init() {
  if (config.input_type == WII) {
    input = new WiiExtension();
  } else if (config.input_type == DIRECT) {
    input = new Direct();
  } else {
    return;
  }
  input->init();
  if (config.input_type == WII || config.tilt_type == MPU_6050) {
    I2Cdev::TWIInit();
    _delay_ms(500);
  }
  IO::enableADC();
  // TODO: move tilt stuff to a dedicated guitar handler
  if (config.tilt_type == MPU_6050) {
    mympu_open(15);
    EICRA = 0;
    bit_set(EIMSK, INT0);
    IO::pinMode(2, INPUT_PULLUP);
  }
}

volatile bool ready = false;
void InputHandler::processTilt() {
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
      controller.r_y = z;
    }
    // controller.r_x = rand() * 100;
  }
}

ISR(INT0_vect) { ready = true; }
