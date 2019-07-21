#pragma once
#include "output_handler.h"

#define SIMULTANEOUS_KEYS 6

#define CHECK_JOY_KEY(joy)                                                     \
  if (usedKeys < SIMULTANEOUS_KEYS) {                                          \
    if (config.keys.joy.neg &&                                                 \
        controller.joy < -(int)config.threshold_joy) {                    \
      KeyboardReport->KeyCode[usedKeys++] = config.keys.joy.neg;               \
    }                                                                          \
    if (config.keys.joy.pos &&                                                 \
        controller.joy > (int)config.threshold_joy) {                     \
      KeyboardReport->KeyCode[usedKeys++] = config.keys.joy.pos;               \
    }                                                                          \
  }

#define CHECK_TRIGGER_KEY(trigger)                                             \
  if (usedKeys < SIMULTANEOUS_KEYS) {                                          \
    if (config.keys.trigger &&                                                 \
        controller.trigger > (int)config.threshold_trigger) {             \
      KeyboardReport->KeyCode[usedKeys++] = config.keys.trigger;               \
    }                                                                          \
  }

void keyboard_init(event_pointers *events, USB_ClassInfo_HID_Device_t *hid_device);