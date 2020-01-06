#pragma once
#include "output_handler.h"

#define SIMULTANEOUS_KEYS 6
#define CHECK_JOY_KEY(joy) check_joy_key(config.keys.joy.neg, config.keys.joy.pos, controller.joy, (int)config.axis.threshold_joy, &usedKeys, KeyboardReport)
#define CHECK_TRIGGER_KEY(trigger) check_joy_key(0, config.keys.trigger, controller.trigger, (int)config.axis.threshold_trigger, &usedKeys, KeyboardReport)

void keyboard_init(void);