#pragma once
#include "../controller/controller.h"
#define CHECK_JOY(joy, neg, pos)                                               \
  if (controller->joy < -joyThreshold) {                           \
    bit_set(controller->buttons, neg);                                          \
  }                                                                            \
  if (controller->joy > joyThreshold) {                            \
    bit_set(controller->buttons, pos);                                          \
  }

void initInputs(void);
void tickInputs(Controller_t* controller);
uint8_t getVelocity(Controller_t* controller, uint8_t offset);
