#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "io.h"
#include "pico_slave.h"

#ifdef SLAVE_TWI_PORT
void slavePinMode(uint8_t pin, uint8_t pinMode) {
    uint8_t payload[] = {pin, pinMode};
    twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_PINMODE, 2, payload);
}

uint32_t slaveReadDigital() {
    uint32_t payload;
    twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL, sizeof(uint32_t), (uint8_t*)&payload);
    return payload;
}

uint16_t slaveReadAnalog(uint8_t pin) {
    uint16_t payload;
    twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_ANALOG, sizeof(uint16_t), (uint8_t*)&payload);
    return payload;
}

void slaveWriteDigital(uint8_t pin, bool output) {
    uint8_t payload[] = {pin, output};
    twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_PIN, 2, payload);
}

uint16_t slaveReadMultiplexer(uint8_t pin, uint8_t channel, uint8_t s0, uint8_t s1, uint8_t s2) {
    slaveWriteDigital(s0, ((channel & (1 << 0)) != 0));
    slaveWriteDigital(s1, ((channel & (1 << 1)) != 0));
    slaveWriteDigital(s2, ((channel & (1 << 2)) != 0));
    return slaveReadAnalog(pin);
}

uint16_t slaveReadMultiplexer(uint8_t pin, uint8_t channel, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3) {
    slaveWriteDigital(s0, ((channel & (1 << 0)) != 0));
    slaveWriteDigital(s1, ((channel & (1 << 1)) != 0));
    slaveWriteDigital(s2, ((channel & (1 << 2)) != 0));
    slaveWriteDigital(s3, ((channel & (1 << 3)) != 0));
    return slaveReadAnalog(pin);
}

void slaveInitLED(uint8_t instance) {
    twi_writeSingleToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INIT_SPI, instance);
}

void slaveWriteLED(uint8_t data) {
    twi_writeSingleToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_WRITE_SPI, data);
}
#endif