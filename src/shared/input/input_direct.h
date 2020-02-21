#pragma once
#include "../controller/controller.h"
#include "pins/pins.h"
#define READ_JOY(axisId)                                                         \
  if (config.pins.axisId.pin != INVALID_PIN) {                                       \
    controller->axisId = (config.pins.axisId.inverted ? -1 : 1) *                     \
                       ((analogRead(config.pins.axisId.pin) - 512) * 64);            \
  }

#define DEFINE_JOY(axisId)                                                       \
  if (config.pins.axisId.pin != INVALID_PIN) pinMode(config.pins.axisId.pin, INPUT)
void direct_init(void);
void direct_tick(controller_t *controller);