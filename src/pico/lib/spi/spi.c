
#include "spi/spi.h"
#include <math.h>
#include <stdio.h>
#include "timer/timer.h"
#include "util/util.h"
#include "pins_arduino.h"
#include "hardware/spi.h"

void spi_begin(uint32_t clock, bool cpol, bool cpha) {
  spi_init(spi0, clock);
  spi_set_format(spi0, 8, cpol, cpha, SPI_MSB_FIRST);
}
uint8_t spi_transfer(uint8_t data) {
  return spi_write_read_blocking(spi0, &data, &data, 1);
}