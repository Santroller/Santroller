#include <avr/io.h>
#include <avr/power.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>

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

void spi_begin(uint32_t clock, bool cpol, bool cpha, bool lsbfirst) {
  SPI_DDR |= (1 << CS) | (1 << MOSI) | (1 << SCK);
  uint8_t config;
  if (cpol) {
    config |= _BV(CPOL);
  }
  if (cpha) {
    config |= _BV(CPHA);
  }
  if (lsbfirst) {
    config |= _BV(DORD);
  }
  calculateClock(clock, config);
}
uint8_t spi_transfer(uint8_t data) {
  SPDR = data;
  asm volatile("nop");
  while (!(SPSR & _BV(SPIF)))
    ;
  return SPDR;
}
void spi_high(void) {
  SPI_PORT |= _BV(SCK) | _BV(MOSI);
}
void spi_low(void) {
  SPI_PORT &= ~(_BV(SCK) | _BV(MOSI));
}