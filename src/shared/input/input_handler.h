#pragma once
#include "../controller/controller.h"
#define CHECK_JOY(joy, neg, pos)                                               \
  if (controller->joy < -jth) {                           \
    bit_set(controller->buttons, neg);                                          \
  }                                                                            \
  if (controller->joy > jth) {                            \
    bit_set(controller->buttons, pos);                                          \
  }

void initInputs(void);
void tickInputs(controller_t* controller);
uint8_t get_value(controller_t* controller, uint8_t offset);