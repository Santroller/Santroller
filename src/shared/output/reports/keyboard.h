#pragma once
#include "controller/controller.h"
#include <stdint.h>
#define SIMULTANEOUS_KEYS 6
#define CHECK_JOY_KEY(joy)                                                     \
  checkJoyKey(config.keys.joy.neg, config.keys.joy.pos, controller->joy,     \
                joyThreshold, &usedKeys, KeyboardReport)
#define CHECK_TRIGGER_KEY(trigger)                                             \
  checkJoyKey(0, config.keys.trigger, controller->trigger, triggerThreshold, \
                &usedKeys, KeyboardReport)

void fillKeyboardReport(void *ReportData, uint16_t *const ReportSize,
                            Controller_t *controller);
void initKeyboard(void);