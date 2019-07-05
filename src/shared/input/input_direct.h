#pragma once
#include "../controller/controller.h"
#include "pins/pins.h"
#define READ_JOY(axis)                                                         \
  if (config.pins.axis != INVALID_PIN) {                                       \
    controller->axis = (config.inversions.axis ? -1 : 1) *                     \
                       ((analogRead(config.pins.axis) - 512) * 64);            \
  }

#define DEFINE_JOY(axis)                                                       \
  if (config.pins.axis != INVALID_PIN) pinMode(config.pins.axis, INPUT)
void direct_init(void);
void direct_tick(controller_t *controller);