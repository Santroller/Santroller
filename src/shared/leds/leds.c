#include "leds.h"
#include "../config/eeprom.h"
#include "arduino_pins.h"
#include "util/util.h"
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

  // enable interrupts
  SPCR |= 1 << SPIE;

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
volatile int currentIndex = -1;
volatile int maxIndex = 0;
uint8_t ledData[((XBOX_BTN_COUNT + XBOX_AXIS_COUNT) + 1) * 4] = {0};
void tickLEDs(Controller_t *controller) {
  // Don't do anything if we are in the middle of writing to the leds at the current moment. Or if the leds are disabled.
  if (config.main.fretLEDMode != APA102 ||
      (currentIndex >= 0 && currentIndex < maxIndex))
    return;
  currentIndex = 0;
  int led = 0;
  // We need to transmit 4 zeros as start bytes, and they are already in ledData, so we just skip to byte 5.
  int idx = 4;
  // Loop until either config.leds runs out, or controller->leds runs out. This
  // is due to the fact that controller->leds can contain more leds if a config
  // is in the process of being made.
  for (; config.leds.pins[led] || controller->leds[led]; led++) {
    uint32_t col = controller->leds[led];
    // Only bind pins to buttons if we know what pin to map, and the computer
    // has not sent a new pin
    if (col == Black && config.leds.pins[led]) {
      uint8_t button = config.leds.pins[led] - 1;
      if (getVelocity(controller, button)) { col = config.leds.colours[led]; }
    }
    ledData[idx++] = 0xff;
    ledData[idx++] = col & 0x0000ff;
    ledData[idx++] = ((col & 0x00ff00) >> 8);
    ledData[idx++] = col >> 16;
  }
  // We need to send the correct amount of stop bytes
  uint8_t stop_bytes = (led + 15) / 16;
  for (uint8_t i = 0; i < stop_bytes; i++) { ledData[idx++] = 0xff; }
  maxIndex = idx;
  // A packet has been created, we need to send the first byte to start transmission.
  SPDR = ledData[currentIndex++];
}
ISR(SPI_STC_vect) {
  if (currentIndex == -1 || currentIndex == maxIndex) return;
  SPDR = ledData[currentIndex++];
}