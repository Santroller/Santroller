#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <hardware/adc.h>

#include "io.h"
#include "pico_slave.h"
volatile uint8_t command;
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
    }
}
void loop() {
}
void setup() {
    Wire1.setSDA(2);
    Wire1.setSCL(3);
    Wire1.begin(SLAVE_ADDR);
    Wire1.onReceive(recv);
    Wire1.onRequest(req);
}