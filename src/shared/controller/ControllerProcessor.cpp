#include "ControllerProcessor.h"
void ControllerProcessor::process() {
  input.read_controller(&controller);
  processTilt();
}
void ControllerProcessor::init() {
  input.init();
#if TILT_SENSOR == MPU_6050
  mympu_open(15);
#elif TILT_SENSOR == GRAVITY
  pinMode(PIN_GRAVITY, INPUT);
#endif
}

void ControllerProcessor::processTilt() {
#if TILT_SENSOR == MPU_6050
  if (counter % 20 == 0) {
    double z;
    mympu_update();
    z = 32767 + (mympu.ypr[2] * (32767 / M_PI));
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
  controller.r_y = direct->digitalRead(PIN_GRAVITY) * 32767;
#endif
}