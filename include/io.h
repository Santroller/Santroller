#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "pins_define.h"

#ifndef TWI_BUFFER_LENGTH
#  define TWI_BUFFER_LENGTH 32
#endif

#define TWI_READY 0
#define TWI_MRX 1
#define TWI_MTX 2
#define TWI_SRX 3
#define TWI_STX 4

#ifdef __cplusplus
extern "C" {
#endif
void read_serial(uint8_t* out, uint8_t len);
void twi_init();
bool twi_readFrom(TWI_BLOCK, uint8_t, uint8_t *, uint8_t, uint8_t);
bool twi_writeTo(TWI_BLOCK, uint8_t, uint8_t *, uint8_t, uint8_t, uint8_t);
bool twi_readFromPointer(TWI_BLOCK, uint8_t address, uint8_t pointer, uint8_t length,
                         uint8_t *data);
bool twi_readSingleFromPointer(TWI_BLOCK, uint8_t address, uint8_t pointer, uint8_t *data);
bool twi_readFromPointerSlow(TWI_BLOCK, uint8_t address, uint8_t pointer, uint8_t length,
                             uint8_t *data);
bool twi_writeSingleToPointer(TWI_BLOCK, uint8_t address, uint8_t pointer, uint8_t data);
bool twi_writeToPointer(TWI_BLOCK, uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data);
void spi_begin();
uint8_t spi_transfer(SPI_BLOCK block, uint8_t data);
void spi_high(SPI_BLOCK block);
void init_ack();
void read_serial(uint8_t* id, uint8_t len);
extern volatile bool spi_acknowledged;
#ifdef __cplusplus
}
#endif