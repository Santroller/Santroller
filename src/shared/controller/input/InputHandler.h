#include "../../config/eeprom.h"
#include "../../io/mpu6050/inv_mpu.h"
#include "../../io/mpu6050/mpu.h"
#include "../../io/pins/Pins.h"
#include "../Controller.h"
#include "../input/DirectInput.h"
#include "../input/WiiExtensionInput.h"
#include "Input.h"

#define CHECK_JOY(joy, neg, pos)                                               \
  if (controller.joy < -(int)config.threshold_joy) {                           \
    bit_set(controller.buttons, neg);                                          \
  }                                                                            \
  if (controller.joy > (int)config.threshold_joy) {                            \
    bit_set(controller.buttons, pos);                                          \
  }

class InputHandler {
public:
  Controller controller;
  Input *input;
  int counter;
  void processTilt();
  void init();
  void process();
};
