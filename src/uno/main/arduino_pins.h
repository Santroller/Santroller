/*
  pins_arduino.h - Pin definition functions for Arduino
  Part of Arduino - http://www.arduino.cc/

  Copyright (c) 2007 David A. Mellis

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
  Boston, MA  02111-1307  USA
*/

#pragma once
#include "Arduino.h"
#include <avr/pgmspace.h>
#include <stdint.h>

#define NUM_DIGITAL_PINS 20
#define NUM_DIGITAL_PINS_NO_DUP NUM_DIGITAL_PINS
#define NUM_ANALOG_INPUTS 6
#define analogInputToDigitalPin(p) ((p < 6) ? (p) + 14 : -1)

#if defined(__AVR_ATmega8__)
#  define digitalPinHasPWM(p) ((p) == 9 || (p) == 10 || (p) == 11)
#else
#  define digitalPinHasPWM(p)                                                  \
    ((p) == 3 || (p) == 5 || (p) == 6 || (p) == 9 || (p) == 10 || (p) == 11)
#endif

#define PIN_SPI_SS (10)
#define PIN_SPI_MOSI (11)
#define PIN_SPI_MISO (12)
#define PIN_SPI_SCK (13)

#define PIN_WIRE_SDA (18)
#define PIN_WIRE_SCL (19)

#define LED_BUILTIN 13

#define PIN_A0 (14)
#define PIN_A1 (15)
#define PIN_A2 (16)
#define PIN_A3 (17)
#define PIN_A4 (18)
#define PIN_A5 (19)
#define PIN_A6 (20)
#define PIN_A7 (21)

#define digitalPinToPCICR(p)                                                   \
  (((p) >= 0 && (p) <= 21) ? (&PCICR) : ((uint8_t *)0))
#define digitalPinToPCICRbit(p) (((p) <= 7) ? 2 : (((p) <= 13) ? 0 : 1))
#define digitalPinToPCMSK(p)                                                   \
  (((p) <= 7) ? (&PCMSK2)                                                      \
              : (((p) <= 13) ? (&PCMSK0)                                       \
                             : (((p) <= 21) ? (&PCMSK1) : ((uint8_t *)0))))
#define digitalPinToPCMSKbit(p)                                                \
  (((p) <= 7) ? (p) : (((p) <= 13) ? ((p)-8) : ((p)-14)))

#define digitalPinToInterrupt(p)                                               \
  ((p) == 2 ? 0 : ((p) == 3 ? 1 : NOT_AN_INTERRUPT))
