#pragma once
#include "controller/controller.h"
#include <stdint.h>
#define CHECK_JOY_KEY(joy)                                                     \
  checkJoyKey(config.keys.joy.neg, config.keys.joy.pos, controller->joy,     \
                joyThresholdKb, &usedKeys, KeyboardReport)
#define CHECK_TRIGGER_KEY(trigger)                                             \
  checkJoyKey(0, config.keys.trigger, controller->trigger, triggerThresholdKb, \
                &usedKeys, KeyboardReport)

void fillKeyboardReport(void *ReportData, uint16_t *const ReportSize,
                            Controller_t *controller);
void initKeyboard(void);