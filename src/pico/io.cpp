#include "io.h"

#include <Wire.h>
#include <math.h>
#include <pico/unique_id.h>
#include <stdio.h>

#include "Arduino.h"
#include "commands.h"
#include "config.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico_slave.h"
#include "wii.h"
volatile bool spi_acknowledged = false;
void spi_begin_output() {
#ifdef SPI_0_OUTPUT
#ifdef SPI_0_MOSI
    spi_init(spi0, SPI_0_CLOCK);
    spi_set_format(spi0, 8, (spi_cpol_t)SPI_0_CPOL,
                   (spi_cpha_t)SPI_0_CPHA, SPI_MSB_FIRST);
    gpio_set_function(SPI_0_MOSI, GPIO_FUNC_SPI);
#ifdef SPI_0_SCK
    gpio_set_function(SPI_0_SCK, GPIO_FUNC_SPI);
#endif
#ifdef SPI_0_MISO
    gpio_set_function(SPI_0_MISO, GPIO_FUNC_SPI);
    gpio_set_pulls(SPI_0_MISO, true, false);
#endif
#endif
    spi_set_slave(spi0, true);
#endif
#ifdef SPI_1_OUTPUT
#ifdef SPI_1_MOSI
    spi_init(spi1, SPI_1_CLOCK);
    spi_set_format(spi1, 8, (spi_cpol_t)SPI_1_CPOL,
                   (spi_cpha_t)SPI_1_CPHA, SPI_MSB_FIRST);
    gpio_set_function(SPI_1_MOSI, GPIO_FUNC_SPI);
#ifdef SPI_1_SCK
    gpio_set_function(SPI_1_SCK, GPIO_FUNC_SPI);
#endif
#ifdef SPI_1_MISO
    gpio_set_function(SPI_1_MISO, GPIO_FUNC_SPI);
    gpio_set_pulls(SPI_1_MISO, true, false);
#endif
#endif
    spi_set_slave(spi1, true);
#endif
}
void spi_begin() {
#ifdef SPI_0_MOSI
    spi_init(spi0, SPI_0_CLOCK);
    spi_set_format(spi0, 8, (spi_cpol_t)SPI_0_CPOL,
                   (spi_cpha_t)SPI_0_CPHA, SPI_MSB_FIRST);
    gpio_set_function(SPI_0_MOSI, GPIO_FUNC_SPI);
#ifdef SPI_0_SCK
    gpio_set_function(SPI_0_SCK, GPIO_FUNC_SPI);
#endif
#ifdef SPI_0_MISO
    gpio_set_function(SPI_0_MISO, GPIO_FUNC_SPI);
    gpio_set_pulls(SPI_0_MISO, true, false);
#endif
#endif
#ifdef SPI_1_MOSI
    spi_init(spi1, SPI_1_CLOCK);
    spi_set_format(spi1, 8, (spi_cpol_t)SPI_1_CPOL,
                   (spi_cpha_t)SPI_1_CPHA, SPI_MSB_FIRST);
    gpio_set_function(SPI_1_MOSI, GPIO_FUNC_SPI);
#ifdef SPI_1_SCK
    gpio_set_function(SPI_1_SCK, GPIO_FUNC_SPI);
#endif
#ifdef SPI_1_MISO
    gpio_set_function(SPI_1_MISO, GPIO_FUNC_SPI);
    gpio_set_pulls(SPI_1_MISO, true, false);
#endif
#endif
#ifdef SPI_0_OUTPUT
    spi_set_slave(spi0, true);
#endif
#ifdef SPI_1_OUTPUT
    spi_set_slave(spi1, true);
#endif
}
static uint8_t revbits(uint8_t b) {
    b = (b & 0b11110000) >> 4 | (b & 0b00001111) << 4;
    b = (b & 0b11001100) >> 2 | (b & 0b00110011) << 2;
    b = (b & 0b10101010) >> 1 | (b & 0b01010101) << 1;
    return b;
}

volatile bool alarm_fired = false;
#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0
int64_t alarm_irq(alarm_id_t id, void *user_data) {
    alarm_fired = true;
    return 0;
}
int __not_in_flash_func(spi_write_read_blocking_timeout)(spi_inst_t *spi, const uint8_t src) {
    // For PS2 communication we NEED timeouts. We can fake them by starting an alarm and resetting the SPI
    // hardware if the alarm fires.
    size_t rx_remaining = 1, tx_remaining = 1;
    uint8_t dst;
    alarm_fired = false;
    alarm_id_t alarm = add_alarm_in_us(50, alarm_irq, NULL, false);
    while (rx_remaining || tx_remaining) {
        if (tx_remaining && spi_is_writable(spi)) {
            spi_get_hw(spi)->dr = (uint32_t)src;
            --tx_remaining;
        }
        if (rx_remaining && spi_is_readable(spi)) {
            dst = (uint8_t)spi_get_hw(spi)->dr;
            --rx_remaining;
        }
        if (alarm_fired) {
            spi_begin_output();
            return 0;
        }
    }
    cancel_alarm(alarm);
    return dst;
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
#ifdef SPI_1_OUTPUT
    if (block == spi1) {
        resp = spi_write_read_blocking_timeout(block, data);
    }
#endif
#ifndef SPI_1_OUTPUT
    if (block == spi1) {
        spi_write_read_blocking(block, &data, &resp, 1);
    }
#endif
#ifdef SPI_0_OUTPUT
    if (block == spi0) {
        resp = spi_write_read_blocking_timeout(block, data);
    }
#endif
#ifndef SPI_0_OUTPUT
    if (block == spi0) {
        spi_write_read_blocking(block, &data, &resp, 1);
    }
#endif
#if SPI_0_MSBFIRST == 0
    if (block == spi0) resp = revbits(resp);
#endif
#if SPI_1_MSBFIRST == 0
    if (block == spi1) resp = revbits(resp);
#endif
    return resp;
}
void spi_high(SPI_BLOCK block) {}
uint8_t addr;
void recv(int len) {
    addr = RXWIRE.read();
    // remove addr
    len -= 1;
    if (len) {
        for (int i = 0; i < len; i++) {
            recv_data(addr + i, RXWIRE.read());
        }
        recv_end(addr, len);
    }
}

// Called when the I2C slave is read from
void req() {
    // Auto increment address for repeated reads
    RXWIRE.write(req_data(addr++));
}
void twi_init() {
#ifdef TWI_0_CLOCK
    i2c_init(i2c0, TWI_0_CLOCK);
    gpio_set_function(TWI_0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TWI_0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TWI_0_SDA);
    gpio_pull_up(TWI_0_SCL);
#endif
#ifdef TWI_1_CLOCK
    i2c_init(i2c1, TWI_1_CLOCK);
    gpio_set_function(TWI_1_SDA, GPIO_FUNC_I2C);
    gpio_set_function(TWI_1_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(TWI_1_SDA);
    gpio_pull_up(TWI_1_SCL);
#endif

#ifdef TWI_1_OUTPUT
    RXWIRE.setSDA(TWI_1_SDA);
    RXWIRE.setSCL(TWI_1_SCL);
    i2c1->hw->enable = 0;
    hw_set_bits(&i2c1->hw->con, I2C_IC_CON_RX_FIFO_FULL_HLD_CTRL_BITS);
    i2c1->hw->enable = 1;
    RXWIRE.begin(WII_ADDR);
    RXWIRE.onReceive(recv);
    RXWIRE.onRequest(req);
#endif
#ifdef TWI_0_OUTPUT
    RXWIRE.setSDA(TWI_0_SDA);
    RXWIRE.setSCL(TWI_0_SCL);
    i2c0->hw->enable = 0;
    hw_set_bits(&i2c0->hw->con, I2C_IC_CON_RX_FIFO_FULL_HLD_CTRL_BITS);
    i2c0->hw->enable = 1;
    RXWIRE.begin(WII_ADDR);
    RXWIRE.onReceive(recv);
    RXWIRE.onRequest(req);
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
        i2c_read_timeout_us(block, address, data, length, !sendStop, 5000);
    return ret > 0 ? ret : 0;
}

bool twi_writeTo(TWI_BLOCK block, uint8_t address, const uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) {
    int ret =
        i2c_write_timeout_us(block, address, data, length, !sendStop, 5000);
    if (ret < 0)
        ret = i2c_write_timeout_us(block, address, data, length, !sendStop,
                                   5000);
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

void read_serial(uint8_t *id, uint8_t len) {
    pico_get_unique_board_id_string((char *)id, len);
}