#pragma once
#include "Output.h"
#include "HidOutput.h"
#include <avr/wdt.h>
#include <stdint.h>
#include <LUFA/Drivers/USB/USB.h>
#define SIMULTANEOUS_KEYS 6

#define CHECK_KEY(key)                                                         \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key;                                              \
  }
#define CHECK_FRET(key)                                                        \
  if (bit_check(controller.buttons, key)) {                                    \
    keys[usedKeys++] = KEY_##key##_FRET;                                       \
  }
#define CHECK_KEY2(key, condition)                                             \
  if (bit_check(controller.buttons, key) || condition) {                       \
    keys[usedKeys++] = KEY_##key;                                              \
  }
extern "C" {
extern uint8_t keys[SIMULTANEOUS_KEYS];
extern uint8_t usedKeys;
}