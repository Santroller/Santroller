
#include "spi/spi.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "timer/timer.h"
#include "util/util.h"
#include <math.h>
#include <stdio.h>
volatile bool spi_acknowledged = false;
void spi_begin(uint32_t clock, bool cpol, bool cpha, bool lsbfirst) {
  // LSBFIRST isnt supported here (also, we may just drop using this and only
  // use PIO)
  spi_init(spi0, clock);
  spi_set_format(spi0, 8, cpol ? SPI_CPOL_1 : SPI_CPOL_0,
                 cpha ? SPI_CPHA_1 : SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
  gpio_set_pulls(PIN_SPI_MISO, true, false);
}
uint8_t revbits2(uint8_t b) {
  b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
  b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
  b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
  return b;
}
uint8_t spi_transfer(uint8_t data) {
  data = revbits2(data);
  uint8_t resp;
  spi_write_read_blocking(spi0, &data, &resp, 1);
  resp = revbits2(resp);
  return resp;
}
void callback (uint gpio, uint32_t events) {
  spi_acknowledged = true;
}
void init_ack(Pin_t ack) {
  gpio_set_irq_enabled_with_callback(ack.pin, GPIO_IRQ_EDGE_RISE, true, &callback);
}
void spi_high(void) {}