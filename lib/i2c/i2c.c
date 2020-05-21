/*
  twi.c - TWI/I2C library for Wiring & Arduino
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

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified 2019 by IanSC to return after a timeout period
*/

#include <avr/interrupt.h>
#include <avr/io.h>
#include <compat/twi.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

// === MODIFIED ===
#include "util/util.h"
#include <util/delay.h>

#include "i2c.h"

static volatile uint8_t i2cState;
static volatile uint8_t i2cSlarw;
static volatile uint8_t i2cSendStop;   // should the transaction end with a stop
static volatile uint8_t i2cInRepStart; // in the middle of a repeated start

static uint8_t i2cMasterBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t i2cMasterBufferIndex;
static volatile uint8_t i2cMasterBufferLength;

static uint8_t i2cTxBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t i2cTxBufferIndex;
static volatile uint8_t i2cTxBufferLength;

static uint8_t i2cRxBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t i2cRxBufferIndex;

static volatile uint8_t i2cError;

// === MODIFIED ===
static uint16_t TIMEOUT = 10000;

/*
 * Function initI2C
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void initI2C(void) {
  // initialize state
  i2cState = TWI_READY;
  i2cSendStop = true; // default value
  i2cInRepStart = false;

// // activate internal pullups for twi.
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega8__) ||                  \
    defined(__AVR_ATmega328P__)
  // activate internal pull-ups for twi
  // as per note from atmega8 manual pg167
  sbi(PORTC, 4);
  sbi(PORTC, 5);
#else
  // activate internal pull-ups for twi
  // as per note from atmega128 manual pg204
  sbi(PORTD, 0);
  sbi(PORTD, 1);
#endif

  // initialize twi prescaler and bit rate
  cbi(TWSR, TWPS0);
  cbi(TWSR, TWPS1);
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

  /* twi bit rate formula from atmega128 manual pg 204
  SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
  note: TWBR should be 10 or higher for master mode
  It is 72 for a 16mhz Wiring board with 100kHz TWI */

  // enable twi module, acks, and twi interrupt
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
}

/*
 * Function disableI2C
 * Desc     disables twi pins
 * Input    none
 * Output   none
 */
void disableI2C(void) {
  // disable twi module, acks, and twi interrupt
  TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));

  // deactivate internal pullups for twi.
  // digitalWrite(SDA, 0);
  // digitalWrite(SCL, 0);
}

/*
 * Function twi_slaveInit
 * Desc     sets slave address and enables interrupt
 * Input    none
 * Output   none
 */
void setI2CAddress(uint8_t address) {
  // set twi slave address (skip over TWGCE bit)
  TWAR = address << 1;
}

/*
 * Function twi_setClock
 * Desc     sets twi bit rate
 * Input    Clock Frequency
 * Output   none
 */
void setI2CFrequency(uint32_t frequency) {
  TWBR = ((F_CPU / frequency) - 16) / 2;

  /* twi bit rate formula from atmega128 manual pg 204
  SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
  note: TWBR should be 10 or higher for master mode
  It is 72 for a 16mhz Wiring board with 100kHz TWI */
}

/*
 * Function readFromI2C
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
// === MODIFIED ===
bool readFromI2C(uint8_t address, uint8_t *data, uint8_t length,
                 uint8_t sendStop) {
  uint8_t i;

  // ensure data will fit into buffer
  if (TWI_BUFFER_LENGTH < length) return false;

  uint16_t timeoutCounter;

  // === MODIFIED ===
  // wait until twi is ready, become master receiver
  if (TIMEOUT == 0) {
    while (TWI_READY != i2cState) continue;
  } else {
    timeoutCounter = TIMEOUT;
    while (TWI_READY != i2cState) {
      //_delay_ms( TIMEOUT_TICK );
      timeoutCounter--;
      if (timeoutCounter == 0) return false;
    }
  }
  /*
  // wait until twi is ready, become master receiver
  while(TWI_READY != twi_state){
    continue;
  }
  */

  i2cState = TWI_MRX;
  i2cSendStop = sendStop;
  // reset error state (0xFF.. no error occured)
  i2cError = 0xFF;

  // initialize buffer iteration vars
  i2cMasterBufferIndex = 0;
  i2cMasterBufferLength = length - 1; // This is not intuitive, read on...
  // On receive, the previously configured ACK/NACK setting is transmitted in
  // response to the received byte before the interrupt is signalled.
  // Therefor we must actually set NACK when the _next_ to last byte is
  // received, causing that NACK to be sent in response to receiving the last
  // expected byte of data.

  // build sla+w, slave device address + w bit
  i2cSlarw = TW_READ;
  i2cSlarw |= address << 1;

  if (true == i2cInRepStart) {
    // if we're in the repeated start state, then we've already sent the start,
    // (@@@ we hope), and the TWI statemachine is just waiting for the address
    // byte. We need to remove ourselves from the repeated start state before we
    // enable interrupts, since the ISR is ASYNC, and we could get confused if
    // we hit the ISR before cleaning up. Also, don't enable the START
    // interrupt. There may be one pending from the repeated start that we sent
    // ourselves, and that would really confuse things.
    i2cInRepStart = false; // remember, we're dealing with an ASYNC ISR

    // === MODIFIED ===
    if (TIMEOUT == 0) {
      do { TWDR = i2cSlarw; } while (TWCR & _BV(TWWC));
    } else {
      timeoutCounter = TIMEOUT;
      TWDR = i2cSlarw;
      while (TWCR & _BV(TWWC)) {
        //_delay_ms( TIMEOUT_TICK );
        timeoutCounter--;
        if (timeoutCounter == 0) return false;
        TWDR = i2cSlarw;
      }
    }
    /*
    do {
      TWDR = i2cSlarw;
    } while(TWCR & _BV(TWWC));
    */
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) |
           _BV(TWIE); // enable INTs, but not START
  } else {
    // send start condition
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);
  }

  // === MODIFIED ===
  // wait for read operation to complete
  if (TIMEOUT == 0) {
    while (TWI_MRX == i2cState) continue;
  } else {
    timeoutCounter = TIMEOUT;
    while (TWI_MRX == i2cState) {
      //_delay_ms( TIMEOUT_TICK );
      timeoutCounter--;
      if (timeoutCounter == 0) return false;
    }
  }
  /*
  // wait for read operation to complete
  while(TWI_MRX == twi_state){
    continue;
  }
  */

  if (i2cMasterBufferIndex < length) length = i2cMasterBufferIndex;

  // copy twi buffer to data
  for (i = 0; i < length; ++i) { data[i] = i2cMasterBuffer[i]; }

  return true;
}

/*
 * Function writeToI2C
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the
 * end Output   0 .. success 1 .. length too long for buffer 2 .. address send,
 * NACK received 3 .. data send, NACK received 4 .. other twi error (lost bus
 * arbitration, bus error, ..)
 */
bool writeToI2C(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                uint8_t sendStop) {
  uint8_t i;

  // ensure data will fit into buffer
  if (TWI_BUFFER_LENGTH < length) return false;

  // === MODIFIED ===
  uint16_t timeoutCounter;
  // wait until twi is ready, become master transmitter
  if (TIMEOUT == 0) {
    while (TWI_READY != i2cState) continue;
  } else {
    timeoutCounter = TIMEOUT;
    while (TWI_READY != i2cState) {
      //_delay_ms( TIMEOUT_TICK );
      timeoutCounter--;
      if (timeoutCounter == 0) return false;
    }
  }
  /*
  // wait until twi is ready, become master transmitter
  while( TWI_READY != i2cState ) {
    continue;
  }
  */

  i2cState = TWI_MTX;
  i2cSendStop = sendStop;
  // reset error state (0xFF.. no error occured)
  i2cError = 0xFF;

  // initialize buffer iteration vars
  i2cMasterBufferIndex = 0;
  i2cMasterBufferLength = length;

  // copy data to twi buffer
  for (i = 0; i < length; ++i) i2cMasterBuffer[i] = data[i];

  // build sla+w, slave device address + w bit
  i2cSlarw = TW_WRITE;
  i2cSlarw |= address << 1;

  // if we're in a repeated start, then we've already sent the START
  // in the ISR. Don't do it again.
  if (true == i2cInRepStart) {
    // if we're in the repeated start state, then we've already sent the start,
    // (@@@ we hope), and the TWI statemachine is just waiting for the address
    // byte. We need to remove ourselves from the repeated start state before we
    // enable interrupts, since the ISR is ASYNC, and we could get confused if
    // we hit the ISR before cleaning up. Also, don't enable the START
    // interrupt. There may be one pending from the repeated start that we sent
    // outselves, and that would really confuse things.
    i2cInRepStart = false; // remember, we're dealing with an ASYNC ISR

    // === MODIFIED ===
    if (TIMEOUT == 0) {
      do { TWDR = i2cSlarw; } while (TWCR & _BV(TWWC));
    } else {
      timeoutCounter = TIMEOUT;
      TWDR = i2cSlarw;
      while (TWCR & _BV(TWWC)) {
        //_delay_ms( TIMEOUT_TICK );
        timeoutCounter--;
        if (timeoutCounter == 0) return false;
        TWDR = i2cSlarw;
      }
    }
    /*
    do {
      TWDR = i2cSlarw;
    } while(TWCR & _BV(TWWC));
    */

    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) |
           _BV(TWIE); // enable INTs, but not START
  } else {
    // send start condition
    TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) |
           _BV(TWSTA); // enable INTs
  }

  // === MODIFIED ===
  // wait for write operation to complete
  if (TIMEOUT == 0) {
    while (wait && (TWI_MTX == i2cState)) continue;
  } else {
    timeoutCounter = TIMEOUT;
    while (wait && (TWI_MTX == i2cState)) {
      //_delay_ms( TIMEOUT_TICK );
      timeoutCounter--;
      if (timeoutCounter == 0) return false;
    }
  }
  /*
  // wait for write operation to complete
  while(wait && (TWI_MTX == i2cState)){
    continue;
  }*/

  if (i2cError == 0xFF) return true; // success
  return false;
}
/*
 * Function sendI2CReply
 * Desc     sends byte or readys receive line
 * Input    ack: byte indicating to ack or to nack
 * Output   none
 */
void sendI2CReply(uint8_t ack) {
  // transmit master read ready signal, with or without ack
  if (ack) {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
  } else {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
  }
}

/*
 * Function sendI2CStop
 * Desc     relinquishes bus master status
 * Input    none
 * Output   none
 */
// === MODIFIED ===
bool sendI2CStop(void) {
  // send stop condition
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);

  // === MODIFIED ===
  // wait for stop condition to be exectued on bus
  // TWINT is not set after a stop condition!
  if (TIMEOUT == 0) {
    while (TWCR & _BV(TWSTO)) continue;
  } else {
    uint16_t timeoutCounter = TIMEOUT;
    while (TWCR & _BV(TWSTO)) {
      //_delay_ms( TIMEOUT_TICK );
      timeoutCounter--;
      if (timeoutCounter == 0) return false;
    }
  }
  /*
  while(TWCR & _BV(TWSTO)){
    continue;
  }
  */

  // update twi state
  i2cState = TWI_READY;
  return true;
}

/*
 * Function releaseI2CBus
 * Desc     releases bus control
 * Input    none
 * Output   none
 */
void releaseI2CBus(void) {
  // release bus
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT);

  // update twi state
  i2cState = TWI_READY;
}
bool readFromI2CPointerSlow(uint8_t address, uint8_t pointer, uint8_t length,
                            uint8_t *data) {
  uint8_t ret = writeToI2C(address, &pointer, 1, true, true);
  if (!ret) return ret;
  _delay_us(200);
  return readFromI2C(address, data, length, true);
}
bool readFromI2CPointer(uint8_t address, uint8_t pointer, uint8_t length,
                        uint8_t *data) {
  return writeToI2C(address, &pointer, 1, true, true) &&
         readFromI2C(address, data, length, true);
}
bool writeSingleToI2CPointer(uint8_t address, uint8_t pointer, uint8_t data) {
  return writeToI2CPointer(address, pointer, 1, &data);
}
bool writeToI2CPointer(uint8_t address, uint8_t pointer, uint8_t length,
                       uint8_t *data) {
  uint8_t data2[length + 1];
  data2[0] = pointer;
  memcpy(data2, data, length);
  return writeToI2C(address, data2, length + 1, true, true);
}
ISR(TWI_vect) {
  switch (TW_STATUS) {
  // All Master
  case TW_START:     // sent start condition
  case TW_REP_START: // sent repeated start condition
    // copy device address and r/w bit to output register and ack
    TWDR = i2cSlarw;
    sendI2CReply(1);
    break;

  // Master Transmitter
  case TW_MT_SLA_ACK:  // slave receiver acked address
  case TW_MT_DATA_ACK: // slave receiver acked data
    // if there is data to send, send it, otherwise stop
    if (i2cMasterBufferIndex < i2cMasterBufferLength) {
      // copy data to output register and ack
      TWDR = i2cMasterBuffer[i2cMasterBufferIndex++];
      sendI2CReply(1);
    } else {
      if (i2cSendStop)
        sendI2CStop();
      else {
        i2cInRepStart = true; // we're gonna send the START
        // don't enable the interrupt. We'll generate the start, but we
        // avoid handling the interrupt until we're in the next transaction,
        // at the point where we would normally issue the start.
        TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
        i2cState = TWI_READY;
      }
    }
    break;
  case TW_MT_SLA_NACK: // address sent, nack received
    i2cError = TW_MT_SLA_NACK;
    sendI2CStop();
    break;
  case TW_MT_DATA_NACK: // data sent, nack received
    i2cError = TW_MT_DATA_NACK;
    sendI2CStop();
    break;
  case TW_MT_ARB_LOST: // lost bus arbitration
    i2cError = TW_MT_ARB_LOST;
    releaseI2CBus();
    break;

  // Master Receiver
  case TW_MR_DATA_ACK: // data received, ack sent
    // put byte into buffer
    i2cMasterBuffer[i2cMasterBufferIndex++] = TWDR;
  case TW_MR_SLA_ACK: // address sent, ack received
    // ack if more bytes are expected, otherwise nack
    if (i2cMasterBufferIndex < i2cMasterBufferLength) {
      sendI2CReply(1);
    } else {
      sendI2CReply(0);
    }
    break;
  case TW_MR_DATA_NACK: // data received, nack sent
    // put final byte into buffer
    i2cMasterBuffer[i2cMasterBufferIndex++] = TWDR;
    if (i2cSendStop)
      sendI2CStop();
    else {
      i2cInRepStart = true; // we're gonna send the START
      // don't enable the interrupt. We'll generate the start, but we
      // avoid handling the interrupt until we're in the next transaction,
      // at the point where we would normally issue the start.
      TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
      i2cState = TWI_READY;
    }
    break;
  case TW_MR_SLA_NACK: // address sent, nack received
    sendI2CStop();
    break;
  // TW_MR_ARB_LOST handled by TW_MT_ARB_LOST case

  // Slave Receiver
  case TW_SR_SLA_ACK:            // addressed, returned ack
  case TW_SR_GCALL_ACK:          // addressed generally, returned ack
  case TW_SR_ARB_LOST_SLA_ACK:   // lost arbitration, returned ack
  case TW_SR_ARB_LOST_GCALL_ACK: // lost arbitration, returned ack
    // enter slave receiver mode
    i2cState = TWI_SRX;
    // indicate that rx buffer can be overwritten and ack
    i2cRxBufferIndex = 0;
    sendI2CReply(1);
    break;
  case TW_SR_DATA_ACK:       // data received, returned ack
  case TW_SR_GCALL_DATA_ACK: // data received generally, returned ack
    // if there is still room in the rx buffer
    if (i2cRxBufferIndex < TWI_BUFFER_LENGTH) {
      // put byte in buffer and ack
      i2cRxBuffer[i2cRxBufferIndex++] = TWDR;
      sendI2CReply(1);
    } else {
      // otherwise nack
      sendI2CReply(0);
    }
    break;
  case TW_SR_STOP: // stop or repeated start condition received
    // ack future responses and leave slave receiver state
    releaseI2CBus();
    // put a null char after data if there's room
    if (i2cRxBufferIndex < TWI_BUFFER_LENGTH) {
      i2cRxBuffer[i2cRxBufferIndex] = '\0';
    }
    // since we submit rx buffer to "wire" library, we can reset it
    i2cRxBufferIndex = 0;
    break;
  case TW_SR_DATA_NACK:       // data received, returned nack
  case TW_SR_GCALL_DATA_NACK: // data received generally, returned nack
    // nack back at master
    sendI2CReply(0);
    break;

  // Slave Transmitter
  case TW_ST_SLA_ACK:          // addressed, returned ack
  case TW_ST_ARB_LOST_SLA_ACK: // arbitration lost, returned ack
    // enter slave transmitter mode
    i2cState = TWI_STX;
    // ready the tx buffer index for iteration
    i2cTxBufferIndex = 0;
    // set tx buffer length to be zero, to verify if user changes it
    i2cTxBufferLength = 0;
    // request for txBuffer to be filled and length to be set
    // if they didn't change buffer & length, initialize it
    if (0 == i2cTxBufferLength) {
      i2cTxBufferLength = 1;
      i2cTxBuffer[0] = 0x00;
    }
    // transmit first byte from buffer, fall
  case TW_ST_DATA_ACK: // byte sent, ack returned
    // copy data to output register
    TWDR = i2cTxBuffer[i2cTxBufferIndex++];
    // if there is more to send, ack, otherwise nack
    if (i2cTxBufferIndex < i2cTxBufferLength) {
      sendI2CReply(1);
    } else {
      sendI2CReply(0);
    }
    break;
  case TW_ST_DATA_NACK: // received nack, we are done
  case TW_ST_LAST_DATA: // received ack, but we are done already!
    // ack future responses
    sendI2CReply(1);
    // leave slave receiver state
    i2cState = TWI_READY;
    break;

  // All
  case TW_NO_INFO: // no state information
    break;
  case TW_BUS_ERROR: // bus error, illegal stop/start
    i2cError = TW_BUS_ERROR;
    sendI2CStop();
    break;
  }
}

// === ADDED/MODIFIED ===
void twi_setTimeout(uint16_t timeout) { TIMEOUT = timeout; }
