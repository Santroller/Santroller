
#include "spi/spi.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>

volatile bool spi_acknowledged = false;
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
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(PIN_SPI_MISO, INPUT_PULLUP);
  pinMode(PIN_SPI_SCK, OUTPUT);
  digitalWrite(PIN_SPI_SS, 1);
  pinMode(PIN_SPI_SS, OUTPUT);
  uint8_t config = 0;
  if (cpol) { config |= _BV(CPOL); }
  if (cpha) { config |= _BV(CPHA); }
  if (lsbfirst) { config |= _BV(DORD); }
  calculateClock(clock, config);
}
uint8_t spi_transfer(uint8_t data) {
  SPDR = data;
  asm volatile("nop");
  while (!(SPSR & _BV(SPIF)))
    ;
  return SPDR;
}
void init_ack(Pin_t ack) {
  // ack is pin 7
  cli();
  EICRA |= INTERRUPT_PS2_ACK_EICRA;
  EIMSK |= INTERRUPT_PS2_ACK_BIT;
  sei();
  // Set up a pin rise interrupt for the ack pin
  // we probably could do it with a pin change interrupt over a pin interrupt
  // depending on the effort
}
void spi_high(void) {
  digitalWrite(PIN_SPI_SCK, true);
  digitalWrite(PIN_SPI_MOSI, true);
}

#ifndef RF_TX
// The PS2 interrupts conflict with RF, but that doesn't matter anyways since so
// does every other pin
// On the uno / mega, we use this pin for RF stuff, and since an interrupt can only be defined once, we use it for both places.
extern bool rf_interrupt;
ISR(INTERRUPT_PS2_ACK) {
  spi_acknowledged = true;
#  ifndef __AVR_ATmega32U4__
  rf_interrupt = true;
#  endif
}
#endif