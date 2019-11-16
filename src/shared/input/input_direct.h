#pragma once
#include "../controller/controller.h"
#include "pins/pins.h"
#define READ_JOY(axisId)                                                         \
  if (config.pins.axisId != INVALID_PIN) {                                       \
    controller->axisId = (config.axis.inversions.axisId ? -1 : 1) *                     \
                       ((analogRead(config.pins.axisId) - 512) * 64);            \
  }

#define DEFINE_JOY(axisId)                                                       \
  if (config.pins.axisId != INVALID_PIN) pinMode(config.pins.axisId, INPUT)
void direct_init(void);
void direct_tick(controller_t *controller);