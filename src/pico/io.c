#include "io.h"

#include <math.h>
#include <stdio.h>

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
volatile bool spi_acknowledged = false;
void spi_begin() {
#ifdef SPI_0_MOSI
    spi_init(spi0, SPI_0_CLOCK);
    spi_set_format(spi0, 8, SPI_0_CPOL,
                   SPI_0_CPHA, SPI_MSB_FIRST);
    gpio_set_function(SPI_0_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SPI_0_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_0_SCK, GPIO_FUNC_SPI);
    gpio_set_pulls(SPI_0_MISO, true, false);
#endif
#ifdef SPI_1_MOSI
    spi_init(spi1, SPI_1_CLOCK);
    spi_set_format(spi1, 8, SPI_1_CPOL,
                   SPI_1_CPHA, SPI_MSB_FIRST);
    gpio_set_function(SPI_1_MISO, GPIO_FUNC_SPI);
    gpio_set_function(SPI_1_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SPI_1_SCK, GPIO_FUNC_SPI);
    gpio_set_pulls(SPI_1_MISO, true, false);
#endif
}
static uint8_t revbits(uint8_t b) {
    b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
    b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
    b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
    return b;
}
// SINCE LSB_FIRST isn't supported, we need to invert bits ourselves when its set
uint8_t spi_transfer(SPI_BLOCK block, uint8_t data) {
#if SPI_0_MSBFIRST == 0
    if (block == spi0) data = revbits(data);
#endif
#if SPI_1_MSBFIRST == 0
    if (block == spi1) data = revbits(data);
#endif
    uint8_t resp;
    spi_write_read_blocking(block, &data, &resp, 1);
#if SPI_0_MSBFIRST == 0
    if (block == spi0) resp = revbits(resp);
#endif
#if SPI_1_MSBFIRST == 0
    if (block == spi1) resp = revbits(resp);
#endif
    return resp;
}
void spi_high(SPI_BLOCK block) {}
void twi_init() {
#ifdef TWI_0_FREQ
    i2c_init(i2c0, TWI_0_FREQ);
    gpio_set_function(TWI_0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TWI_0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TWI_0_SDA);
    gpio_pull_up(TWI_0_SCL);
#endif
#ifdef TWI_1_FREQ
    i2c_init(i2c1, TWI_1_FREQ);
    gpio_set_function(TWI_1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TWI_1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TWI_1_SDA);
    gpio_pull_up(TWI_1_SCL);
#endif
}
bool twi_readFromPointerSlow(TWI_BLOCK block, uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data) {
  if (!twi_writeTo(block, address, &pointer, 1, true, true)) return false;
  delayMicroseconds(170);
  return twi_readFrom(block, address, data, length, true);
}
bool twi_readFrom(TWI_BLOCK block, uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop) {
    int ret =
        i2c_read_timeout_us(block, address, data, length, !sendStop, 1000);
    return ret > 0 ? ret : 0;
}

bool twi_writeTo(TWI_BLOCK block, uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) {
    int ret =
        i2c_write_timeout_us(block, address, data, length, !sendStop, 1000);
    if (ret < 0)
        ret = i2c_write_timeout_us(block, address, data, length, !sendStop,
                                   1000);
    return ret > 0;
}

#ifdef PS2_ACK
void callback(uint gpio, uint32_t events) {
    spi_acknowledged = true;
}
void init_ack() {
    gpio_set_irq_enabled_with_callback(PS2_ACK, GPIO_IRQ_EDGE_RISE, true, &callback);
}
#endif