#pragma once
#include "../../../config/config.h"
#include "../Controller.h"
#if OUTPUT_TYPE == XINPUT
#include "XInputOutput.h"
#elif OUTPUT_TYPE == KEYBOARD
#include "KeyboardOutput.h"
#endif
#include "../util.h"
class OutputHandler {
public:
#if OUTPUT_TYPE == XINPUT
  XInputOutput output;
#elif OUTPUT_TYPE == KEYBOARD
  KeyboardOutput output;
#endif
  void process(Controller *controller);
  void init();
};