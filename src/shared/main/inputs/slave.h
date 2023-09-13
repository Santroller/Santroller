#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "io.h"
#include "pico_slave.h"
#include "commands.h"

#ifdef SLAVE_TWI_PORT
void slavePinMode(uint8_t pin, uint8_t pinMode) {
    uint8_t payload[] = {pin, pinMode};
    slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_PINMODE, sizeof(payload), payload);
    // Give pins some time to configure themselves
    delayMicroseconds(100);
}

uint32_t slaveReadDigital() {
    uint32_t payload;
    slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL, sizeof(payload), (uint8_t*)&payload);
    return payload;
}

uint16_t slaveReadAnalog(uint8_t pin) {
    uint8_t payload2[] = {pin};
    slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_ANALOG_PIN, sizeof(payload2), payload2);
    uint16_t payload;
    slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_ANALOG, sizeof(payload), (uint8_t*)&payload);
    return payload;
}

uint8_t slaveReadDigital(uint8_t port, uint8_t mask) {
    uint8_t payload2[] = {port, mask};
    slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL_PIN_2, sizeof(payload2), payload2);
    uint32_t payload;
    slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL, sizeof(payload), (uint8_t*)&payload);
    PIN_INIT;
    return ((payload >> port) & 0xff);
}

uint16_t slaveReadAnalog(uint8_t pin, uint8_t mask) {
    slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_ANALOG_PIN_2, sizeof(pin), &pin);
    uint16_t payload;
    slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_ANALOG, sizeof(payload), (uint8_t*)&payload);
    bool detecting = pin & (1 << 7);
    if (detecting) {
        PIN_INIT;
    }
    return payload;
}

void slaveWriteDigital(uint8_t pin, bool output) {
    uint8_t payload[] = {pin, output};
    twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_PIN, sizeof(payload), payload);
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
    slave_initted = twi_writeSingleToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INIT_SPI, instance);
}

void slaveWriteLED(uint8_t data) {
    slave_initted = twi_writeSingleToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_WRITE_SPI, data);
}

uint8_t slaveReadWt() {
    uint8_t data;
    slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_WT, sizeof(data), &data);
    return data;
}
uint8_t slaveReadWtRaw(uint8_t* output) {
    slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_WT_RAW, 5 * sizeof(uint32_t), output);
    return 5 * sizeof(uint32_t);
}
bool slaveInit() {
    uint8_t data;
    twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INITIALISE, sizeof(data), &data);
    return data == SLAVE_COMMAND_INITIALISE;
}
void slaveInitWt() {
#ifdef INPUT_WT_SLAVE_NECK
    uint8_t payload[] = {WT_PIN_INPUT, WT_PIN_S0, WT_PIN_S1, WT_PIN_S2, WT_SENSITIVITY >> 8, WT_SENSITIVITY & 0xFF, WT_COUNTER >> 8, WT_COUNTER & 0xFF};
    slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INIT_WT, sizeof(payload), payload);
#endif
}
void slaveSetWtCounter(uint16_t counter) {
#ifdef INPUT_WT_SLAVE_NECK
    slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_WT_COUNTER, sizeof(counter), &counter);
#endif
}
#endif