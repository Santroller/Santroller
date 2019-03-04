#include "ControllerProcessor.h"
void ControllerProcessor::process() {
#if DEVICE_TYPE == WII
  extension.read_controller(&controller);
#elif DEVICE_TYPE == DIRECT
  direct.read_controller(&controller);
#endif
  processTilt();
}
void ControllerProcessor::init() {
#if DEVICE_TYPE == WII
  extension.init();
#endif
  mympu_open(15);
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
  controller.r_y = direct->analogueRead(PIN_GRAVITY);
#endif
}