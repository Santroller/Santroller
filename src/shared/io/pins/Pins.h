#pragma once
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "pins.h"

// Enable a pinchangeinterrupt
#define EN_PCI(pin)                                                            \
  *digitalPinToPCMSK(pin) |= (1 << digitalPinToPCMSKbit(pin));                 \
  *digitalPinToPCICR(pin) |= (1 << digitalPinToPCICRbit(pin));                 \
  IO::pinMode(pin, INPUT_PULLUP);

class IO {
public:
  static int digitalRead(uint8_t pin);
  static int analogRead(uint8_t pin);
  static void pinMode(uint8_t pin, uint8_t mode);
  static void enableADC();
};