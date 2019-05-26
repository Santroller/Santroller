#include "../../config/eeprom.h"
#include "../../io/pins/Pins.h"
#include "../Controller.h"
#include "../input/DirectInput.h"
#include "../input/WiiExtensionInput.h"
#include "GuitarHandler.h"
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
  GuitarHandler guitar;
  Controller controller;
  Input *input;
  int counter;
  void processTilt();
  void init();
  void process();
};
