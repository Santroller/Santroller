#pragma once
#include "util/delay.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdbool.h>
#define byte uint8_t
#define boolean bool

// TWI bit rate
#define TWI_FREQ 300000
// Transmit buffer length
#define TXMAXBUFLEN 32
// Receive buffer length
#define RXMAXBUFLEN 32
void i2c_init(void);
int8_t readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, bool isWii);
int8_t readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                 uint8_t *data, bool isWii);

bool writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
bool writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                uint8_t *data);
