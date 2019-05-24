#include "InputHandler.h"
#include "../../io/bootloader/Bootloader.h"
void InputHandler::process() {
  if (input != NULL) {
    input->read_controller(&controller);
    if (config.map_joy_to_dpad) {
      CHECK_JOY(l_x, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT);
      CHECK_JOY(l_y, XBOX_DPAD_UP, XBOX_DPAD_DOWN);
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
  }
  //TODO: move tilt stuff to a dedicated guitar handler
  if (config.tilt_type == MPU_6050) {
    mympu_open(15);
  } 
  IO::enableADC();
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

      if (isnan(z)) {
        z = 0;
      }
      controller.r_y = z;
    }
    counter++;
  } 
}