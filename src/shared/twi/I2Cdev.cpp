#include "I2Cdev.h"
TWIInfoStruct I2Cdev::TWIInfo;
int I2Cdev::RXBuffLen;
int I2Cdev::TXBuffLen;
int I2Cdev::RXBuffIndex;
volatile int I2Cdev::TXBuffIndex;
volatile uint8_t I2Cdev::TWIReceiveBuffer[RXMAXBUFLEN];
uint8_t I2Cdev::TWITransmitBuffer[TXMAXBUFLEN];
/** Default constructor.
 */
I2Cdev::I2Cdev() {}
void I2Cdev::TWIInit() {
  TWIInfo.mode = Ready;
  TWIInfo.errorCode = 0xFF;
  TWIInfo.repStart = 0;
  // Set pre-scalers (no pre-scaling)
  TWSR = 0;
  // Set bit rate
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
  // Enable TWI and interrupt
  TWCR = (1 << TWIE) | (1 << TWEN);
}

uint8_t I2Cdev::isTWIReady() {
  return (TWIInfo.mode == Ready) || (TWIInfo.mode == RepeatedStartSent);
}
uint8_t I2Cdev::TWIWriteRegister(uint8_t device, uint8_t addr, uint8_t value) {
  uint8_t msg[] = {(device << 1) & 0xFE, addr, value};
  TWITransmitData(msg, 3, 0);
  WAIT_TWI;
  return 1;
}
uint8_t I2Cdev::TWIWriteRegisterMultiple(uint8_t device, uint8_t addr,
                                         uint8_t *value, uint8_t bytesToWrite) {
  uint8_t msg[2 + bytesToWrite];
  msg[0] = (device << 1) & 0xFE;
  msg[1] = addr;
  for (int i = 0; i < bytesToWrite; i++) {
    msg[i + 2] = value[i];
  }

  TWITransmitData(msg, bytesToWrite + 2, 0);
  WAIT_TWI;
  return 1;
}
uint8_t I2Cdev::TWIReadRegister(uint8_t device, uint8_t address,
                                uint8_t bytesToRead) {
  uint8_t msg[] = {(device << 1) & 0xFE, address};
  TWITransmitData(msg, 2, 0);
  WAIT_TWI;
  TWIReadData(device, bytesToRead, 0);
  WAIT_TWI;
  return 1;
}
uint8_t I2Cdev::TWITransmitData(void *const TXdata, uint8_t dataLen,
                                uint8_t repStart) {
  if (dataLen <= TXMAXBUFLEN) {
    // Wait until ready
    WAIT_TWI;
    // Set repeated start mode
    TWIInfo.repStart = repStart;
    // Copy data into the transmit buffer
    uint8_t *data = (uint8_t *)TXdata;
    for (int i = 0; i < dataLen; i++) {
      TWITransmitBuffer[i] = data[i];
    }
    // Copy transmit info to global variables
    TXBuffLen = dataLen;
    TXBuffIndex = 0;

    // If a repeated start has been sent, then devices are already listening for
    // an address and another start does not need to be sent.
    if (TWIInfo.mode == RepeatedStartSent) {
      TWIInfo.mode = Initializing;
      TWDR = TWITransmitBuffer[TXBuffIndex++]; // Load data to transmit buffer
      TWISendTransmit();                       // Send the data
    } else // Otherwise, just send the normal start signal to begin
           // transmission.
    {
      TWIInfo.mode = Initializing;
      TWISendStart();
    }
  } else {
    return 1; // return an error if data length is longer than buffer
  }
  return 0;
}

uint8_t I2Cdev::TWIReadData(uint8_t TWIaddr, uint8_t bytesToRead,
                            uint8_t repStart) {
  // Check if number of bytes to read can fit in the RXbuffer
  if (bytesToRead < RXMAXBUFLEN) {
    // Reset buffer index and set RXBuffLen to the number of bytes to read
    RXBuffIndex = 0;
    RXBuffLen = bytesToRead;
    // Create the one value array for the address to be transmitted
    uint8_t TXdata[1];
    // Shift the address and AND a 1 into the read write bit (set to write mode)
    TXdata[0] = (TWIaddr << 1) | 0x01;
    // Use the TWITransmitData function to initialize the transfer and address
    // the slave
    TWITransmitData(TXdata, 1, repStart);
  } else {
    return 0;
  }
  return 1;
}
/** Read a single bit from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-7)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t I2Cdev::readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum,
                       uint8_t *data, uint16_t timeout) {
  uint8_t b;
  uint8_t count = readByte(devAddr, regAddr, &b, timeout);
  *data = b & (1 << bitNum);
  return count;
}

/** Read a single bit from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-15)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t I2Cdev::readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum,
                        uint16_t *data, uint16_t timeout) {
  uint16_t b;
  uint8_t count = readWord(devAddr, regAddr, &b, timeout);
  *data = b & (1 << bitNum);
  return count;
}

/** Read multiple bits from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @param data Container for right-aligned value (i.e. '101' read from any
 * bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t I2Cdev::readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart,
                        uint8_t length, uint8_t *data, uint16_t timeout) {
  // 01101001 read byte
  // 76543210 bit numbers
  //    xxx   args: bitStart=4, length=3
  //    010   masked
  //   -> 010 shifted
  uint8_t count, b;
  if ((count = readByte(devAddr, regAddr, &b, timeout)) != 0) {
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    b &= mask;
    b >>= (bitStart - length + 1);
    *data = b;
  }
  return count;
}

/** Read multiple bits from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-15)
 * @param length Number of bits to read (not more than 16)
 * @param data Container for right-aligned value (i.e. '101' read from any
 * bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (1 = success, 0 = failure, -1 = timeout)
 */
int8_t I2Cdev::readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart,
                         uint8_t length, uint16_t *data, uint16_t timeout) {
  // 1101011001101001 read byte
  // fedcba9876543210 bit numbers
  //    xxx           args: bitStart=12, length=3
  //    010           masked
  //           -> 010 shifted
  uint8_t count;
  uint16_t w;
  if ((count = readWord(devAddr, regAddr, &w, timeout)) != 0) {
    uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    w &= mask;
    w >>= (bitStart - length + 1);
    *data = w;
  }
  return count;
}

/** Read single byte from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for byte value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t I2Cdev::readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data,
                        uint16_t timeout) {
  return readBytes(devAddr, regAddr, 1, data, timeout);
}

/** Read single word from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for word value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t I2Cdev::readWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data,
                        uint16_t timeout) {
  return readWords(devAddr, regAddr, 1, data, timeout);
}

/** Read multiple bytes from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Number of bytes read (-1 indicates failure)
 */
int8_t I2Cdev::readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                         uint8_t *data, uint16_t timeout) {
  TWIReadRegister(devAddr, regAddr, length);
  for (int i = 0; i < length; i++) {
    data[i] = TWIReceiveBuffer[i];
  }
  return length;
}

/** Read multiple words from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of words to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in I2Cdev::readTimeout)
 * @return Number of words read (-1 indicates failure)
 */
int8_t I2Cdev::readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                         uint16_t *data, uint16_t timeout) {
  TWIReadRegister(devAddr, regAddr, length * 2);
  for (int i = 0; i < length * 2; i += 2) {
    data[i] = TWIReceiveBuffer[i] << 8 | TWIReceiveBuffer[i + 1];
  }
  return length;
}

/** write a single bit in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-7)
 * @param value New bit value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum,
                      uint8_t data) {
  uint8_t b;
  readByte(devAddr, regAddr, &b);
  b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
  return writeByte(devAddr, regAddr, b);
}

/** write a single bit in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-15)
 * @param value New bit value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum,
                       uint16_t data) {
  uint16_t w;
  readWord(devAddr, regAddr, &w);
  w = (data != 0) ? (w | (1 << bitNum)) : (w & ~(1 << bitNum));
  return writeWord(devAddr, regAddr, w);
}

/** Write multiple bits in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart,
                       uint8_t length, uint8_t data) {
  //      010 value to write
  // 76543210 bit numbers
  //    xxx   args: bitStart=4, length=3
  // 00011100 mask byte
  // 10101111 original value (sample)
  // 10100011 original & ~mask
  // 10101011 masked | value
  uint8_t b;
  if (readByte(devAddr, regAddr, &b) != 0) {
    uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1); // shift data into correct position
    data &= mask;                     // zero all non-important bits in data
    b &= ~(mask); // zero all important bits in existing byte
    b |= data;    // combine data with existing byte
    return writeByte(devAddr, regAddr, b);
  } else {
    return false;
  }
}

/** Write multiple bits in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-15)
 * @param length Number of bits to write (not more than 16)
 * @param data Right-aligned value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart,
                        uint8_t length, uint16_t data) {
  //              010 value to write
  // fedcba9876543210 bit numbers
  //    xxx           args: bitStart=12, length=3
  // 0001110000000000 mask word
  // 1010111110010110 original value (sample)
  // 1010001110010110 original & ~mask
  // 1010101110010110 masked | value
  uint16_t w;
  if (readWord(devAddr, regAddr, &w) != 0) {
    uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
    data <<= (bitStart - length + 1); // shift data into correct position
    data &= mask;                     // zero all non-important bits in data
    w &= ~(mask); // zero all important bits in existing word
    w |= data;    // combine data with existing word
    return writeWord(devAddr, regAddr, w);
  } else {
    return false;
  }
}

/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
  return writeBytes(devAddr, regAddr, 1, &data);
}

/** Write single word to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New word value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data) {
  return writeWords(devAddr, regAddr, 1, &data);
}

/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                        uint8_t *data) {
  return TWIWriteRegisterMultiple(devAddr, regAddr, data, length);
}

/** Write multiple words to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of words to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                        uint16_t *data) {
  return TWIWriteRegisterMultiple(devAddr, regAddr, (uint8_t *)data,
                                  length * 2);
}
void I2Cdev::interrupt() {
  switch (TWI_STATUS) {
  // ----\/ ---- MASTER TRANSMITTER OR WRITING ADDRESS ----\/ ----  //
  case TWI_MT_SLAW_ACK: // SLA+W transmitted and ACK received
    // Set mode to Master Transmitter
    TWIInfo.mode = MasterTransmitter;
  case TWI_START_SENT:           // Start condition has been transmitted
  case TWI_MT_DATA_ACK:          // Data byte has been transmitted, ACK received
    if (TXBuffIndex < TXBuffLen) // If there is more data to send
    {
      TWDR = TWITransmitBuffer[TXBuffIndex++]; // Load data to transmit buffer
      TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
      TWISendTransmit(); // Send the data
    }
    // This transmission is complete however do not release bus yet
    else if (TWIInfo.repStart) {
      TWIInfo.errorCode = 0xFF;
      TWISendStart();
    }
    // All transmissions are complete, exit
    else {
      TWIInfo.mode = Ready;
      TWIInfo.errorCode = 0xFF;
      TWISendStop();
    }
    break;

    // ----\/ ---- MASTER RECEIVER ----\/ ----  //

  case TWI_MR_SLAR_ACK: // SLA+R has been transmitted, ACK has been received
    // Switch to Master Receiver mode
    TWIInfo.mode = MasterReceiver;
    // If there is more than one byte to be read, receive data byte and return
    // an ACK
    if (RXBuffIndex < RXBuffLen - 1) {
      TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
      TWISendACK();
    }
    // Otherwise when a data byte (the only data byte) is received, return NACK
    else {
      TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
      TWISendNACK();
    }
    break;

  case TWI_MR_DATA_ACK: // Data has been received, ACK has been transmitted.

    /// -- HANDLE DATA BYTE --- ///
    TWIReceiveBuffer[RXBuffIndex++] = TWDR;
    // If there is more than one byte to be read, receive data byte and return
    // an ACK
    if (RXBuffIndex < RXBuffLen - 1) {
      TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
      TWISendACK();
    }
    // Otherwise when a data byte (the only data byte) is received, return NACK
    else {
      TWIInfo.errorCode = TWI_NO_RELEVANT_INFO;
      TWISendNACK();
    }
    break;

  case TWI_MR_DATA_NACK: // Data byte has been received, NACK has been
                         // transmitted. End of transmission.

    /// -- HANDLE DATA BYTE --- ///
    TWIReceiveBuffer[RXBuffIndex++] = TWDR;
    // This transmission is complete however do not release bus yet
    if (TWIInfo.repStart) {
      TWIInfo.errorCode = 0xFF;
      TWISendStart();
    }
    // All transmissions are complete, exit
    else {
      TWIInfo.mode = Ready;
      TWIInfo.errorCode = 0xFF;
      TWISendStop();
    }
    break;

    // ----\/ ---- MT and MR common ----\/ ---- //

  case TWI_MR_SLAR_NACK: // SLA+R transmitted, NACK received
  case TWI_MT_SLAW_NACK: // SLA+W transmitted, NACK received
  case TWI_MT_DATA_NACK: // Data byte has been transmitted, NACK received
  case TWI_LOST_ARBIT:   // Arbitration has been lost
    // Return error and send stop and set mode to ready
    if (TWIInfo.repStart) {
      TWIInfo.errorCode = TWI_STATUS;
      TWISendStart();
    }
    // All transmissions are complete, exit
    else {
      TWIInfo.mode = Ready;
      TWIInfo.errorCode = TWI_STATUS;
      TWISendStop();
    }
    break;
  case TWI_REP_START_SENT: // Repeated start has been transmitted
    // Set the mode but DO NOT clear TWINT as the next data is not yet ready
    TWIInfo.mode = RepeatedStartSent;
    break;

  // ----\/ ---- SLAVE RECEIVER ----\/ ----  //

  // TODO  IMPLEMENT SLAVE RECEIVER FUNCTIONALITY

  // ----\/ ---- SLAVE TRANSMITTER ----\/ ----  //

  // TODO  IMPLEMENT SLAVE TRANSMITTER FUNCTIONALITY

  // ----\/ ---- MISCELLANEOUS STATES ----\/ ----  //
  case TWI_NO_RELEVANT_INFO: // It is not really possible to get into this ISR
                             // on this condition Rather, it is there to be
                             // manually set between operations
    break;
  case TWI_ILLEGAL_START_STOP: // Illegal START/STOP, abort and return error
    TWIInfo.errorCode = TWI_ILLEGAL_START_STOP;
    TWIInfo.mode = Ready;
    TWISendStop();
    break;
  }
}
/** Default timeout value for read operations.
 * Set this to 0 to disable timeout detection.
 */
uint16_t I2Cdev::readTimeout = I2CDEV_DEFAULT_READ_TIMEOUT;
ISR(TWI_vect) { I2Cdev::interrupt(); }
