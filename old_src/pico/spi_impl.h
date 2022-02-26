#include <math.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pins.h"
#include "timer.h"

void spi_begin(uint32_t clock, bool cpol, bool cpha, bool lsbfirst) {
    // We dont have the ability to control lsbfirst

    spi_init(spi0, clock);
    spi_set_format(spi0, 8, cpol ? SPI_CPOL_1 : SPI_CPOL_0,
                   cpha ? SPI_CPHA_1 : SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SPI_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
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
    spi_write_read_blocking(spi0, &data, &resp, 1);
    resp = revbits2(resp);
    return resp;
}
void spi_high(void) {
    // TODO: is this necessary?
}
void spi_low(void) {
    // TODO: is this necessary?
}
