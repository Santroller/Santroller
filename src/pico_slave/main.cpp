#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <hardware/adc.h>

#include "io.h"
#include "pico_slave.h"
#ifdef TWI_1_SDA
#define WIRE Wire1
#else
#define WIRE Wire
#endif
volatile uint8_t command;
volatile uint8_t wtInputPin = 0;
volatile uint32_t mask = 0;
volatile uint8_t wtS0Pin = 0;
volatile uint8_t wtS1Pin = 0;
volatile uint8_t wtS2Pin = 0;
volatile uint16_t wt_sensitivity = 0;
volatile uint8_t rawWt;
volatile uint32_t lastWt[5] = {0};
volatile bool hasInitWt = false;
spi_inst_t* hardware;
void recv(int len) {
    command = WIRE.read();
    switch (command) {
        case SLAVE_COMMAND_SET_PINMODE: {
            uint8_t pin = WIRE.read();
            uint8_t mode = WIRE.read();
            switch (mode) {
                case PIN_MODE_INPUT_PULLDOWN:
                    gpio_init(pin);
                    gpio_set_dir(pin, false);
                    gpio_set_pulls(pin, false, true);
                case PIN_MODE_INPUT_PULLUP:
                    gpio_init(pin);
                    gpio_set_dir(pin, false);
                    gpio_set_pulls(pin, true, false);
                    break;
                case PIN_MODE_INPUT:
                    gpio_init(pin);
                    gpio_set_dir(pin, false);
                    gpio_set_pulls(pin, false, false);
                    break;
                case PIN_MODE_INPUT_BUSKEEP:
                    gpio_init(pin);
                    gpio_set_dir(pin, false);
                    gpio_set_pulls(pin, true, true);
                    break;
                case PIN_MODE_OUTPUT:
                    gpio_init(pin);
                    gpio_set_dir(pin, true);
                    break;
                case PIN_MODE_ANALOG:
                    adc_gpio_init(pin);
                    break;
                case PIN_MODE_SPI:
                    gpio_set_function(pin, GPIO_FUNC_SPI);
                    break;
            }
            break;
        }
        case SLAVE_COMMAND_SET_PIN:
            digitalWrite(WIRE.read(), WIRE.read());
            break;
        case SLAVE_COMMAND_GET_DIGITAL_PIN_2: {
            uint8_t port = WIRE.read() * 8;
            uint32_t mask32 = WIRE.read() << port;
            for (uint i = 0; i < NUM_BANK0_GPIOS; i++) {
#ifdef TWI_1_SDA
                if (i == TWI_1_SDA || i == TWI_1_SCL) {
                    continue;
                }
#endif
#ifdef TWI_0_SDA
                if (i == TWI_0_SDA || i == TWI_0_SCL) {
                    continue;
                }
#endif
                if (mask32 & (1 << i)) {
                    gpio_init(i);
                    gpio_set_pulls(i, true, false);
                }
            }
            break;
        }
        case SLAVE_COMMAND_INIT_SPI:
            if (WIRE.read()) {
                hardware = spi1;
            } else {
                hardware = spi0;
            }
            spi_init(hardware, F_CPU / 2);
            spi_set_format(hardware, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
            break;
        case SLAVE_COMMAND_WRITE_SPI: {
            uint8_t data = WIRE.read();
            spi_write_blocking(hardware, &data, 1);
            break;
        }
        case SLAVE_COMMAND_INIT_WT:
            wtInputPin = WIRE.read();
            wtS0Pin = WIRE.read();
            wtS1Pin = WIRE.read();
            wtS2Pin = WIRE.read();
            mask = (1 << wtS0Pin) | (1 << wtS1Pin) | (1 << wtS2Pin);
            wt_sensitivity = WIRE.read() << 8 | WIRE.read();
            hasInitWt = false;
            break;
    }
}

// Called when the I2C slave is read from
void req() {
    switch (command) {
        case SLAVE_COMMAND_GET_DIGITAL: {
            uint32_t pins = sio_hw->gpio_in;
            WIRE.write((uint8_t*)&pins, sizeof(pins));
            break;
        }
        case SLAVE_COMMAND_GET_WT: {
            WIRE.write((uint8_t*)&rawWt, sizeof(rawWt));
            break;
        }
        case SLAVE_COMMAND_GET_WT_RAW: {
            volatile uint8_t* data = (volatile uint8_t*)lastWt;
            for (uint8_t i = 0; i < sizeof(lastWt); i++) {
                WIRE.write(data[i]);
            }
            break;
        }
        case SLAVE_COMMAND_INITIALISE: {
            uint8_t ret = SLAVE_COMMAND_INITIALISE;
            WIRE.write(&ret, sizeof(ret));
            break;
        }
    }
}
#define WT_BUFFER 16
uint32_t lastWtSum[5] = {0};
uint32_t lastWtAvg[5][WT_BUFFER] = {0};
uint8_t nextWt[5] = {0};
uint32_t initialWt[5] = {0};
uint32_t readWtSlave(int pin) {
    gpio_put_masked(mask, ((pin & (1 << 0)) << wtS0Pin - 0) | ((pin & (1 << 1)) << (wtS1Pin - 1)) | ((pin & (1 << 2)) << (wtS2Pin - 2)));
    sleep_us(10);
    gpio_set_dir(wtInputPin, true);
    gpio_put(wtInputPin, 0);
    gpio_set_dir(wtInputPin, false);
    gpio_set_pulls(wtInputPin, true, false);
    uint32_t m = rp2040.getCycleCount();
    while (!gpio_get(wtInputPin)) {
    }
    m = rp2040.getCycleCount() - m;
    if (pin < 6) {
        if (m > 1000) {
            m = initialWt[pin] - wt_sensitivity;
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
    }
    return m;
}
bool checkWtSlave(int pin) {
    return readWtSlave(pin) > initialWt[pin];
}

void loop() {
    if (wtS2Pin && !hasInitWt) {
        hasInitWt = true;
        memset(initialWt, 0, sizeof(initialWt));
        memset(lastWtAvg, 0, sizeof(lastWtAvg));
        memset(lastWtSum, 0, sizeof(lastWtSum));
        memset(nextWt, 0, sizeof(nextWt));
        for (int j = 0; j < 100; j++) {
            readWtSlave(6);
            for (int i = 0; i < 5; i++) {
                readWtSlave(i);
            }
        }
        for (int j = 0; j < 50; j++) {
            readWtSlave(6);
            for (int i = 0; i < 5; i++) {
                uint32_t reading = readWtSlave(i) + wt_sensitivity;
                if (reading > initialWt[i]) {
                    initialWt[i] = reading;
                }
            }
        }
    }
    if (hasInitWt) {
        rawWt = checkWtSlave(1) | (checkWtSlave(0) << 1) | (checkWtSlave(2) << 2) | (checkWtSlave(3) << 3) | (checkWtSlave(4) << 4);
    }
}
void setup() {
#ifdef TWI_1_SDA
    WIRE.setSDA(TWI_1_SDA);
    WIRE.setSCL(TWI_1_SCL);
    gpio_pull_up(TWI_1_SDA);
    gpio_pull_up(TWI_1_SCL);
    i2c1->hw->enable = 0;
    hw_set_bits(&i2c1->hw->con, I2C_IC_CON_RX_FIFO_FULL_HLD_CTRL_BITS);
    i2c1->hw->enable = 1;
#else
    WIRE.setSDA(TWI_0_SDA);
    WIRE.setSCL(TWI_0_SCL);
    gpio_pull_up(TWI_0_SDA);
    gpio_pull_up(TWI_0_SCL);
    i2c0->hw->enable = 0;
    hw_set_bits(&i2c0->hw->con, I2C_IC_CON_RX_FIFO_FULL_HLD_CTRL_BITS);
    i2c0->hw->enable = 1;
#endif
    WIRE.begin(SLAVE_ADDR);
    WIRE.onReceive(recv);
    WIRE.onRequest(req);
}