#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <hardware/adc.h>

#include "io.h"
#include "pico_slave.h"
volatile uint8_t command;
volatile uint8_t wtInputPin = 0;
volatile uint8_t wtS0Pin = 0;
volatile uint8_t wtS1Pin = 0;
volatile uint8_t wtS2Pin = 0;
uint8_t gh5_mapping[] = {
    0x00, 0x95, 0xCD, 0xB0, 0x1A, 0x19, 0xE6,
    0xE5, 0x49, 0x47, 0x48, 0x46, 0x2F, 0x2D,
    0x2E, 0x2C, 0x7F, 0x7B, 0x7D, 0x79, 0x7E,
    0x7A, 0x7C, 0x78, 0x66, 0x62, 0x64, 0x60,
    0x65, 0x61, 0x63, 0x5F};
uint8_t rawWt;
long initialWt[5] = {0};
spi_inst_t* hardware;
void recv(int len) {
    command = Wire1.read();
    switch (command) {
        case SLAVE_COMMAND_SET_PINMODE: {
            uint8_t mode = Wire1.read();
            uint8_t pin = Wire1.read();
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
            digitalWrite(Wire1.read(), Wire1.read());
            break;
        case SLAVE_COMMAND_INIT_SPI:
            if (Wire1.read()) {
                hardware = spi1;
            } else {
                hardware = spi0;
            }
            spi_init(hardware, F_CPU / 2);
            spi_set_format(hardware, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
            break;
        case SLAVE_COMMAND_WRITE_SPI: {
            uint8_t data = Wire1.read();
            uint8_t resp;
            spi_write_read_blocking(hardware, &data, &resp, 1);
            break;
        }
        case SLAVE_COMMAND_GET_ANALOG:
            adc_select_input(Wire1.read());
            break;
        case SLAVE_COMMAND_INIT_WT:
            wtInputPin = Wire1.read();
            wtS0Pin = Wire1.read();
            wtS1Pin = Wire1.read();
            wtS2Pin = Wire1.read();
            break;
    }
}

// Called when the I2C slave is read from
void req() {
    switch (command) {
        case SLAVE_COMMAND_GET_DIGITAL: {
            uint32_t pins = sio_hw->gpio_in;
            Wire1.write((uint8_t*)&pins, sizeof(pins));
            break;
        }
        case SLAVE_COMMAND_GET_ANALOG: {
            uint16_t pins = adc_read() << 4;
            Wire1.write((uint8_t*)&pins, sizeof(pins));
            break;
        }
        case SLAVE_COMMAND_GET_WT_GH5: {
            Wire1.write(gh5_mapping + rawWt, 1);
            break;
        }
        case SLAVE_COMMAND_GET_WT_RAW: {
            Wire1.write(&rawWt, sizeof(rawWt));
            break;
        }
    }
}
long readWt(int pin) {
    digitalWrite(wtS0Pin, pin & 0b001);
    digitalWrite(wtS1Pin, pin & 0b010);
    digitalWrite(wtS2Pin, pin & 0b100);
    long m = 0;
    for (int i = 0; i < 8; i++) {
        pinMode(wtInputPin, INPUT_PULLUP);
        delayMicroseconds(10);
        pinMode(wtInputPin, INPUT);
        m += analogRead(wtInputPin);
    }
    return m;
}
bool checkWt(int pin) {
    return readWt(pin) > initialWt[pin];
}
long lastWtTick = 0;
void loop() {
    if (wtS2Pin && (millis() - lastWtTick) > 1) {
        lastWtTick = millis();
        rawWt = checkWt(4) | (checkWt(3) << 1) | (checkWt(2) << 2) | (checkWt(0) << 3) | (checkWt(1) << 4);
    }
}
void setup() {
    Wire1.setSDA(2);
    Wire1.setSCL(3);
    Wire1.begin(SLAVE_ADDR);
    Wire1.onReceive(recv);
    Wire1.onRequest(req);
}