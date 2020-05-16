#include "leds.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "Arduino.h"
#include "arduino_pins.h"
// #include "input_handler.h"
#include <avr/power.h>
void led_init(void) {
  if (config.main.fret_mode != APA102) return;
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
void apa102_transmit_byte(uint8_t data) {
  SPDR = data;
  while (!(SPSR & _BV(SPIF)))
    ;
}
void led_tick(controller_t *controller) {
  if (config.main.fret_mode != APA102) return;
  for (uint8_t i = 0; i < 4; i++) { apa102_transmit_byte(0x00); }
  int led = 0;
  for (; config.new_items.leds.pins[led]; led++) {
    uint32_t col = controller->leds[led];
    if (col == Black) {
      uint8_t button = config.new_items.leds.pins[led] - 1;
      if (get_value(controller,button)) { col = config.new_items.leds.colours[led]; }
    }
    apa102_transmit_byte(0xff);
    apa102_transmit_byte(col & 0x0000ff);
    apa102_transmit_byte(((col & 0x00ff00) >> 8));
    apa102_transmit_byte(col >> 16);
  }
  uint8_t stop_bytes = (led + 15) / 16;
  for (uint8_t i = 0; i < stop_bytes; i++) { apa102_transmit_byte(0xff); }
}