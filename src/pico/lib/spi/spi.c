
#include "spi/spi.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pins_arduino.h"
#include "spi/pio_spi.h"
#include "timer/timer.h"
#include "util/util.h"
#include <math.h>
#include <stdio.h>
#include "pins/pins.h"

pio_spi_inst_t spi = {.pio = pio0, .sm = 0};
void spi_begin(uint32_t clock, bool cpol, bool cpha, bool lsbfirst) {
  // LSBFIRST isnt supported here (also, we may just drop using this and only
  // use PIO)
  // spi_init(spi0, clock);
  // spi_set_format(spi0, 8, cpol ? SPI_CPOL_1 : SPI_CPOL_0,
  //                cpha ? SPI_CPHA_1 : SPI_CPHA_0, SPI_MSB_FIRST);
  // gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
  // gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
  // gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);

  pinMode(PIN_PS2_ATT, OUTPUT);
  gpio_put(PIN_PS2_ATT, 1);
  float clkdiv = clock_get_hz(clk_sys) / clock;
  uint cpha_prog_offs =
      pio_add_program(spi.pio, cpha ? &spi_cpha1_program : &spi_cpha0_program);
  pio_spi_init(spi.pio, spi.sm, cpha_prog_offs,
               8, // 8 bits per SPI frame
               clkdiv, cpha, cpol, PIN_SPI_SCK, PIN_SPI_MOSI, PIN_SPI_MISO);
}
uint8_t revbits2(uint8_t x) {
  x = (((x & 0xaaU) >> 1) | ((x & 0x55U) << 1));
  x = (((x & 0xccU) >> 2) | ((x & 0x33U) << 2));
  x = (((x & 0xf0U) >> 4) | ((x & 0x0fU) << 4));
  return x;
}
uint8_t spi_transfer(uint8_t data) {
  uint8_t read = data;
  data = revbits2(data);
  uint8_t resp;
  pio_spi_write8_read8_blocking(&spi, &data, &resp, 1);
  resp = revbits2(resp);
  printf("0x%x => 0x%x\n", read, resp);
  return resp;
}