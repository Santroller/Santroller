#pragma once
#include "../controller/controller.h"
#define CHECK_JOY(joy, neg, pos)                                               \
  if (controller->joy < -(int)config.axis.threshold_joy) {                           \
    bit_set(controller->buttons, neg);                                          \
  }                                                                            \
  if (controller->joy > (int)config.axis.threshold_joy) {                            \
    bit_set(controller->buttons, pos);                                          \
  }

void input_init(void);
void input_tick(controller_t* controller);