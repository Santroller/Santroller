
#include "spi/spi.h"
#include <math.h>
#include <stdio.h>
#include "timer/timer.h"
#include "util/util.h"
#include "pins_arduino.h"
#include "hardware/spi.h"

void spi_begin(uint32_t clock, bool cpol, bool cpha) {
  spi_init(spi0, clock);
  gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
  spi_set_format(spi0, 8, cpol, cpha, SPI_MSB_FIRST);
}
uint8_t spi_transfer(uint8_t data) {
  gpio_set_oeover(PIN_SPI_MOSI, GPIO_OVERRIDE_NORMAL);
  gpio_set_oeover(PIN_SPI_SCK, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(PIN_SPI_MOSI, GPIO_OVERRIDE_NORMAL);
  gpio_set_outover(PIN_SPI_SCK, GPIO_OVERRIDE_NORMAL);
  uint8_t resp;
  spi_write_read_blocking(spi0, &data, &resp, 1);
  return resp;
}