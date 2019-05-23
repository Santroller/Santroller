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
  sei();
}

uint8_t I2Cdev::isTWIReady() {
  return (TWIInfo.mode == Ready) || (TWIInfo.mode == RepeatedStartSent);
}
uint8_t I2Cdev::TWIWriteRegister(uint8_t device, uint8_t addr, uint8_t value) {
  uint8_t msg[] = {(uint8_t)((device << 1) & 0xFE), addr, value};
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
  uint8_t msg[] = {(uint8_t)((device << 1) & 0xFE), address};
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
int8_t readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data) {
  return I2Cdev::readBytes(devAddr, regAddr, length, data, 0);
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
/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool I2Cdev::writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
  return writeBytes(devAddr, regAddr, 1, &data);
}
int8_t writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                        uint8_t *data) {
  return I2Cdev::writeBytes(devAddr, regAddr, length, data);
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
