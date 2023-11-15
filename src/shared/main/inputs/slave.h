#include <stdbool.h>
#include <stdint.h>

#include "commands.h"
#include "config.h"
#include "io.h"
#include "pico_slave.h"
#define RETRY_COUNT 2
#ifdef SLAVE_TWI_PORT
void slavePinMode(uint8_t pin, uint8_t pinMode) {
    if (!slave_initted) {
        return;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        uint8_t payload[] = {pin, pinMode};
        slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_PINMODE, sizeof(payload), payload);
        if (slave_initted) {
            break;
        }
    }
}

uint32_t slaveReadDigital() {
    if (!slave_initted) {
        return 0;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        uint32_t payload = 0;
        slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL, sizeof(payload), (uint8_t*)&payload);
        if (slave_initted) {
            return payload;
        }
    }
    return 0;
}

uint8_t slaveReadDigital(uint8_t port, uint8_t mask) {
    if (!slave_initted) {
        return 0;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        uint8_t payload2[2] = {port, mask};
        slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL_PIN_2, sizeof(payload2), payload2);
        if (!slave_initted) {
            continue;
        }
        uint32_t payload = 0;
        slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_DIGITAL, sizeof(payload), (uint8_t*)&payload);
        if (!slave_initted) {
            continue;
        }
        PIN_INIT_PERIPHERAL;
        return (payload >> (port * 8)) & 0xFF;
    }
    return 0;
}

void slaveWriteDigital(uint8_t pin, bool output) {
    if (!slave_initted) {
        return;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        uint8_t payload[] = {pin, output};
        slave_initted = (SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_SET_PIN, sizeof(payload), payload);
        if (slave_initted) {
            return;
        }
    }
}

void slaveInitLED(uint8_t instance) {
    if (!slave_initted) {
        return;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        slave_initted = twi_writeSingleToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INIT_SPI, instance);
        if (slave_initted) {
            return;
        }
    }
}

void slaveWriteLED(uint8_t data) {
    if (!slave_initted) {
        return;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        slave_initted = twi_writeSingleToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_WRITE_SPI, data);
        if (slave_initted) {
            return;
        }
    }
}

uint8_t slaveReadWt() {
    if (!slave_initted) {
        return 0;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        uint8_t data = 0;
        slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_WT, sizeof(data), &data);

        if (slave_initted) {
            return data;
        }
    }
    return 0;
}
uint8_t slaveReadWtRaw(uint8_t* output) {
    if (!slave_initted) {
        return 0;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        slave_initted = twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_GET_WT_RAW, 5 * sizeof(uint32_t), output);
        if (slave_initted) {
            return 5 * sizeof(uint32_t);
        }
    }
    return 0;
}
bool slaveInit() {
    uint8_t data;
    twi_readFromPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INITIALISE, sizeof(data), &data);
    return data == SLAVE_COMMAND_INITIALISE;
}
void slaveInitWt() {
#ifdef INPUT_WT_SLAVE_NECK
    if (!slave_initted) {
        return;
    }
    for (int i = 0; i < RETRY_COUNT; i++) {
        uint8_t payload[] = {WT_PIN_INPUT, WT_PIN_S0, WT_PIN_S1, WT_PIN_S2, WT_SENSITIVITY >> 8, WT_SENSITIVITY & 0xFF};
        slave_initted = twi_writeToPointer(SLAVE_TWI_PORT, SLAVE_ADDR, SLAVE_COMMAND_INIT_WT, sizeof(payload), payload);
        if (slave_initted) {
            return;
        }
    }
#endif
}
#endif