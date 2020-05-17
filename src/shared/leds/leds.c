#include "leds.h"
#include "../config/eeprom.h"
#include "util/util.h"
#include "arduino_pins.h"
// #include "input_handler.h"
#include <avr/power.h>
void initLEDs(void) {
  if (config.main.fretLEDMode != APA102) return;
  clock_prescale_set(clock_div_1);
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(PIN_SPI_MISO, INPUT_PULLUP);
  pinMode(PIN_SPI_SCK, OUTPUT);
  pinMode(PIN_SPI_SS, OUTPUT);
  // enable spi
  SPCR |= (1 << SPE);

  // set as master
  SPCR |= 1 << MSTR;

  // set clock polarity/phase to mode 3
  SPCR |= (1 << CPOL) | (1 << CPHA);

  // set clock scale to 1/2
  SPSR |= 1 << SPI2X;
  SPCR |= (1 << SPR1) | (1 << SPR0);
}
/* Send out data via SPI & wait until transmission is complete */
void transmitSPIByte(uint8_t data) {
  SPDR = data;
  while (!(SPSR & _BV(SPIF)))
    ;
}
void tickLEDs(Controller_t *controller) {
  if (config.main.fretLEDMode != APA102) return;
  for (uint8_t i = 0; i < 4; i++) { transmitSPIByte(0x00); }
  int led = 0;
  for (; config.leds.pins[led]; led++) {
    uint32_t col = controller->leds[led];
    if (col == Black) {
      uint8_t button = config.leds.pins[led] - 1;
      if (getVelocity(controller,button)) { col = config.leds.colours[led]; }
    }
    transmitSPIByte(0xff);
    transmitSPIByte(col & 0x0000ff);
    transmitSPIByte(((col & 0x00ff00) >> 8));
    transmitSPIByte(col >> 16);
  }
  uint8_t stop_bytes = (led + 15) / 16;
  for (uint8_t i = 0; i < stop_bytes; i++) { transmitSPIByte(0xff); }
}