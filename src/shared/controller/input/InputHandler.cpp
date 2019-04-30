#include "InputHandler.h"
#include "../bootloader/Bootloader.h"
void InputHandler::process() {
  if (input != NULL) {
    input->read_controller(&controller);
    processTilt();
  }
}
void InputHandler::init() {
  if (check_serial()) return;
  if (config.input_type == WII) {
    input = new WiiExtension();
  } else if (config.input_type == DIRECT) {
    input = new Direct();
  } else {
    return;
  }
  input->init();
  if (config.tilt_type == MPU_6050) {
    mympu_open(15);
  } else if (config.tilt_type == GRAVITY) {
    IO::pinMode(config.pins.gravity, INPUT);
  }
  if (config.tilt_type == GRAVITY || config.input_type == DIRECT) {
    IO::enableADC();
  }
}

void InputHandler::processTilt() {
  if (config.tilt_type == MPU_6050) {
    if (counter % 20 == 0) {
      double z;
      mympu_update();
      z = (mympu.ypr[2] * (32767 / M_PI));
      z += config.mpu_6050_calibration;
      if (z > 32767) {
        z = 0;
      }
      z = z * 2;
      if (z > 32767) {
        z = 65535 - z;
      }
      z = pow(z, 1.1f);
      z = constrain(z, -32767, 32767);
      controller.r_y = z;
    }
    counter++;
  } else if (config.tilt_type == GRAVITY) {
    controller.r_y = IO::digitalRead(config.pins.gravity) * 32767;
  }
}