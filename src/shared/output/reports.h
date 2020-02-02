#pragma once
#include "../config/defines.h"
#include "../config/eeprom.h"
#include "../controller/controller.h"
#include "controller_structs.h"
#include "stdint.h"
#define COPY(first, second)                                                    \
  bit_write(bit_check(controller.buttons, XBOX_##first),                       \
            JoystickReport->buttons, SWITCH_##second);

#define SIMULTANEOUS_KEYS 6
#define CHECK_JOY_KEY(joy)                                                     \
  check_joy_key(config.keys.joy.neg, config.keys.joy.pos, controller.joy,      \
                (int)config.axis.threshold_joy, &usedKeys, KeyboardReport)
#define CHECK_TRIGGER_KEY(trigger)                                             \
  check_joy_key(0, config.keys.trigger, controller.trigger,                    \
                (int)config.axis.threshold_trigger, &usedKeys, KeyboardReport)

void create_report(void *ReportData, uint16_t *const ReportSize,
                   controller_t controller);
void report_init(void);