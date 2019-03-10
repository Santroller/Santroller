#include "InputHandler.h"
void InputHandler::process() {
  input.read_controller(&controller);
  processTilt();
}
void InputHandler::init() {
  input.init();
#if TILT_SENSOR == MPU_6050
  mympu_open(15);
#elif TILT_SENSOR == GRAVITY
  pinMode(PIN_GRAVITY, INPUT);
#endif
#if TILT_SENSOR == GRAVITY || DEVICE_TYPE == DIRECT
  IO::enableADC();
#endif
}

void InputHandler::processTilt() {
#if TILT_SENSOR == MPU_6050
  if (counter % 20 == 0) {
    double z;
    mympu_update();
    z = (mympu.ypr[2] * (32767 / M_PI));
    z += MPU_6050_START;
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
#elif TILT_SENSOR == GRAVITY
  controller.r_y = IO::digitalRead(PIN_GRAVITY) * 32767;
#endif
}