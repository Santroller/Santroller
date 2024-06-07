#include "io.h"

#include <math.h>
#include <pico/unique_id.h>
#include <stdio.h>
#include <Wire.h>

#include "Arduino.h"
#include "wii.h"
#include "commands.h"
#include "config.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico_slave.h"
volatile bool spi_acknowledged = false;
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
// #ifdef SPI_1_MOSI
//     spi_init(spi1, SPI_1_CLOCK);
//     spi_set_format(spi1, 8, (spi_cpol_t)SPI_1_CPOL,
//                    (spi_cpha_t)SPI_1_CPHA, SPI_MSB_FIRST);
//     gpio_set_function(SPI_1_MOSI, GPIO_FUNC_SPI);
// #ifdef SPI_1_SCK
//     gpio_set_function(SPI_1_SCK, GPIO_FUNC_SPI);
// #endif
// #ifdef SPI_1_MISO
//     gpio_set_function(SPI_1_MISO, GPIO_FUNC_SPI);
//     // gpio_set_pulls(SPI_1_MISO, true, false);
// #endif
// #endif
#ifdef SPI_0_OUTPUT
    spi_set_slave(spi0, true);
#endif
// #ifdef SPI_1_OUTPUT
//     // gpio_set_function(9, GPIO_FUNC_SPI);
//     spi_set_slave(spi1, true);
// #endif
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
uint8_t addr;
void recv(int len) {
    addr = RXWIRE.read();
    // remove addr
    len -= 1;
    if (len) {
        for (int i = 0; i < len; i++) {
            recv_data(addr+i, RXWIRE.read());
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

bool twi_writeTo(TWI_BLOCK block, uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
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

#ifdef INPUT_WT_NECK

#define WT_BUFFER 8
uint32_t lastWt[5] = {0};
uint32_t lastWtSum[5] = {0};
uint32_t lastWtAvg[5][WT_BUFFER] = {0};
uint8_t nextWt[5] = {0};
uint32_t initialWt[5] = {0};
uint32_t readWt(int pin) {
    gpio_put_masked((1 << WT_PIN_S0) | (1 << WT_PIN_S1) | (1 << WT_PIN_S2), ((pin & (1 << 0)) << WT_PIN_S0 - 0) | ((pin & (1 << 1)) << (WT_PIN_S1 - 1)) | ((pin & (1 << 2)) << (WT_PIN_S2 - 2)));
    uint32_t m = rp2040.getCycleCount();
    gpio_put(WT_PIN_INPUT, 1);
    gpio_set_dir(WT_PIN_INPUT, true);
    gpio_set_dir(WT_PIN_INPUT, false);
    gpio_set_pulls(WT_PIN_INPUT, false, false);
    while (gpio_get(WT_PIN_INPUT)) {
        if (rp2040.getCycleCount() - m > 10000) {
            break;
        }
    }
    m = rp2040.getCycleCount() - m;
    if (pin >= 6) {
        return m;
    }
    lastWtSum[pin] -= lastWtAvg[pin][nextWt[pin]];
    lastWtAvg[pin][nextWt[pin]] = m;
    lastWtSum[pin] += m;
    nextWt[pin]++;
    if (nextWt[pin] >= WT_BUFFER) {
        nextWt[pin] = 0;
    }
    m = lastWtSum[pin] / WT_BUFFER;
    lastWt[pin] = m;
    return m;
}
bool checkWt(int pin) {
    return readWt(pin) > initialWt[pin];
}
void initWt() {
    memset(initialWt, 0, sizeof(initialWt));
    for (int j = 0; j < 1000; j++) {
        for (int i = 0; i < 5; i++) {
            initialWt[i] += readWt(i);
        }
    }
    for (int i = 0; i < 5; i++) {
        initialWt[i] /= 1000;
        initialWt[i] += WT_SENSITIVITY;
    }
}
static bool wtInit = false;
uint8_t tickWt() {
    if (!wtInit) {
        wtInit = true;
        initWt();
    }
    long m = micros();
    readWt(6);
    uint8_t ret = checkWt(1) | (checkWt(0) << 1) | (checkWt(2) << 2) | (checkWt(3) << 3) | (checkWt(4) << 4);
    m = micros() - m;
    printf("%d\r\n", m);
    return ret;
}
#endif