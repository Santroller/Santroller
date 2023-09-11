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
volatile uint8_t wtS0Pin = 0;
volatile uint8_t wtS1Pin = 0;
volatile uint8_t wtS2Pin = 0;
volatile uint8_t wt_sensitivity = 0;
uint8_t rawWt;
uint32_t initialWt[5] = {0};
uint32_t lastWt[5] = {0};
bool hasInitWt = false;
bool initted = false;
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
        case SLAVE_COMMAND_GET_ANALOG_PIN: {
            adc_select_input(WIRE.read());
            break;
        }
        case SLAVE_COMMAND_GET_DIGITAL_PIN_2: {
            uint8_t port = WIRE.read() * 8;
            uint32_t mask32 = WIRE.read() << port;
            for (uint i = 0; i < NUM_BANK0_GPIOS; i++) {
                if (mask32 & (1 << i)) {
                    gpio_init(i);
                    gpio_set_pulls(i, true, false);
                }
            }
            break;
        }
        case SLAVE_COMMAND_GET_ANALOG_PIN_2: {
            uint8_t pin = WIRE.read();
            bool detecting = pin & (1 << 7);
            if (detecting) {
                pin = pin & ~(1 << 7);
                gpio_init(pin + PIN_A0);
                gpio_set_pulls(pin + PIN_A0, true, false);
                gpio_set_input_enabled(pin + PIN_A0, false);
            }
            adc_select_input(pin);
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
            uint8_t resp;
            spi_write_read_blocking(hardware, &data, &resp, 1);
            break;
        }
        case SLAVE_COMMAND_INIT_WT:
            wtInputPin = WIRE.read();
            wtS0Pin = WIRE.read();
            wtS1Pin = WIRE.read();
            wtS2Pin = WIRE.read();
            wt_sensitivity = WIRE.read();
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
        case SLAVE_COMMAND_GET_ANALOG: {
            uint16_t pins = adc_read() << 4;
            WIRE.write((uint8_t*)&pins, sizeof(pins));
            break;
        }
        case SLAVE_COMMAND_GET_WT: {
            WIRE.write(&rawWt, sizeof(rawWt));
            break;
        }
        case SLAVE_COMMAND_GET_WT_RAW: {
            WIRE.write((uint8_t*)lastWt, sizeof(lastWt));
            break;
        }
        case SLAVE_COMMAND_INITIALISE: {
            uint8_t ret = SLAVE_COMMAND_INITIALISE;
            WIRE.write(&ret, sizeof(ret));
            break;
        }
    }
}
uint32_t readWtSlave(int pin) {
    digitalWrite(wtS0Pin, pin & 0b001);
    digitalWrite(wtS1Pin, pin & 0b010);
    digitalWrite(wtS2Pin, pin & 0b100);
    uint32_t m = 0;
    for (int i = 0; i < 8; i++) {
        pinMode(wtInputPin, OUTPUT);
        pinMode(wtInputPin, INPUT_PULLUP);
        m += analogRead(wtInputPin);
    }
    return m;
}
bool checkWtSlave(int pin) {
    uint32_t val = readWtSlave(pin);
    lastWt[pin] = val;
    return val > initialWt[pin];
}
uint32_t lastWtTick = 0;
void loop() {
    if (wtS2Pin && !hasInitWt) {
        hasInitWt = true;
        memset(initialWt, 0xFF, sizeof(initialWt));
        for (int j = 0; j < 50; j++) {
            for (int i = 0; i < 5; i++) {
                uint32_t reading = readWtSlave(i) - wt_sensitivity;
                if (reading > initialWt[i]) {
                    initialWt[i] = reading;
                }
            }
        }
    }
    if (hasInitWt && (millis() - lastWtTick) > 1) {
        lastWtTick = millis();
        rawWt = checkWtSlave(1) | (checkWtSlave(0) << 1) | (checkWtSlave(2) << 2) | (checkWtSlave(3) << 3) | (checkWtSlave(4) << 4);
    }
}
void setup() {
#ifdef TWI_1_SDA
    WIRE.setSDA(TWI_1_SDA);
    WIRE.setSCL(TWI_1_SCL);
#else
    WIRE.setSDA(TWI_0_SDA);
    WIRE.setSCL(TWI_0_SCL);
#endif
    WIRE.begin(SLAVE_ADDR);
    WIRE.onReceive(recv);
    WIRE.onRequest(req);
}