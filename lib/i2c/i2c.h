/*
  twi.h - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef twi_h
#define twi_h

#include <inttypes.h>
#include <stdbool.h>

//#define ATMEGA8

#ifndef TWI_FREQ
#  define TWI_FREQ 300000L
#endif

#ifndef TWI_BUFFER_LENGTH
#  define TWI_BUFFER_LENGTH 32
#endif

#define TWI_READY 0
#define TWI_MRX 1
#define TWI_MTX 2
#define TWI_SRX 3
#define TWI_STX 4

void initI2C(void);
void disableI2C(void);
void setI2CAddress(uint8_t);
void setI2CFrequency(uint32_t);
uint8_t readFromI2C(uint8_t, uint8_t *, uint8_t, uint8_t);
uint8_t writeToI2C(uint8_t, uint8_t *, uint8_t, uint8_t, uint8_t);
void sendI2CReply(uint8_t);
bool sendI2CStop(void);
void releaseI2CBus(void);
uint8_t endI2CTransmission(uint8_t txAddress, uint8_t sendStop);
uint8_t readFromI2CPointer(uint8_t address, uint8_t pointer, uint8_t length,
                            uint8_t *data);
uint8_t readFromI2CPointerSlow(uint8_t address, uint8_t pointer,
                                uint8_t length, uint8_t *data);
uint8_t writeToI2CPointer(uint8_t address, uint8_t pointer, uint8_t length,
                           uint8_t *data);
uint8_t writeSingleToI2CPointer(uint8_t address, uint8_t pointer, uint8_t data);

#endif
