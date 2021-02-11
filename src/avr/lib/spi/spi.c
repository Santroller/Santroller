
#include "spi.h"
#include <avr/io.h>
#include <avr/power.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
#include "util/util.h"
#include "pins_arduino.h"

void calculateClock(uint32_t clock, uint8_t config) {
  uint8_t clockDiv;
  if (clock >= F_CPU / 2) {
    clockDiv = 0;
  } else if (clock >= F_CPU / 4) {
    clockDiv = 1;
  } else if (clock >= F_CPU / 8) {
    clockDiv = 2;
  } else if (clock >= F_CPU / 16) {
    clockDiv = 3;
  } else if (clock >= F_CPU / 32) {
    clockDiv = 4;
  } else if (clock >= F_CPU / 64) {
    clockDiv = 5;
  } else {
    clockDiv = 7;
  }

  // Invert the SPI2X bit
  clockDiv ^= 0x1;

  SPCR = _BV(SPE) | _BV(MSTR) | config | ((clockDiv >> 1) & 0x03);
  SPSR = clockDiv & 0x01;
}

void spi_init(uint32_t clock, uint8_t config) {
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(PIN_SPI_MISO, INPUT_PULLUP);
  pinMode(PIN_SPI_SCK, OUTPUT);
  digitalWrite(PIN_SPI_SS, 1);
  pinMode(PIN_SPI_SS, OUTPUT);
  calculateClock(clock, config);
}