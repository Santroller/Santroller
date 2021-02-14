
#include "spi/spi.h"
#include <math.h>
#include <stdio.h>
#include "timer/timer.h"
#include "util/util.h"
#include "pins_arduino.h"
#include "hardware/spi.h"

void spi_begin(uint32_t clock, uint8_t config) {
  spi_init(spi0, clock);
}
uint8_t spi_transfer(uint8_t data) {
  return spi_write_read_blocking(spi0, &data, &data, 1);
}