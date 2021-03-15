
#include "spi/spi.h"
#include "hardware/spi.h"
#include "pins_arduino.h"
#include "timer/timer.h"
#include "util/util.h"
#include <math.h>
#include <stdio.h>

void spi_begin(uint32_t clock, bool cpol, bool cpha) {
  spi_init(spi0, clock);
  spi_set_format(spi0, 8, cpol ? SPI_CPOL_1 : SPI_CPOL_0, cpha ? SPI_CPHA_1 : SPI_CPHA_0, SPI_MSB_FIRST);
  gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
}
uint8_t spi_transfer(uint8_t data) {
  uint8_t resp;
  spi_write_read_blocking(spi0, &data, &resp, 1);
  return resp;
}