#include "i2c_dev.h"

#define WAIT_I2C                                                               \
  while (!isI2CReady()) {                                                      \
    _delay_us(1);                                                              \
  }

// Get TWI status
#define I2C_STATUS (TWSR & 0xF8)
// I2C Status Codes
#define I2C_START_SENT 0x08     // Start sent
#define I2C_REP_START_SENT 0x10 // Repeated Start sent
// Master Transmitter Mode
#define I2C_MT_SLAW_ACK 0x18  // SLA+W sent and ACK received
#define I2C_MT_SLAW_NACK 0x20 // SLA+W sent and NACK received
#define I2C_MT_DATA_ACK 0x28  // DATA sent and ACK received
#define I2C_MT_DATA_NACK 0x30 // DATA sent and NACK received
// Master Receiver Mode
#define I2C_MR_SLAR_ACK 0x40  // SLA+R sent, ACK received
#define I2C_MR_SLAR_NACK 0x48 // SLA+R sent, NACK received
#define I2C_MR_DATA_ACK 0x50  // Data received, ACK returned
#define I2C_MR_DATA_NACK 0x58 // Data received, NACK returned

// Miscellaneous States
#define I2C_LOST_ARBIT 0x38       // Arbitration has been lost
#define I2C_NO_RELEVANT_INFO 0xF8 // No relevant information available
#define I2C_ILLEGAL_START_STOP                                                 \
  0x00 // Illegal START or STOP condition has been detected
#define I2C_SUCCESS 0xFF
// Successful transfer, this state is impossible from TWSR as bit2 is 0 and read
// only

#define I2CSendStart()                                                         \
  (TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN) | (1 << TWIE))
// Send the START signal, enable interrupts and I2C, clear TWINT flag to resume
// transfer.
#define I2CSendStop()                                                          \
  (TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN) | (1 << TWIE))
// Send the STOP signal, enable interrupts and I2C, clear TWINT flag.
#define I2CSendTransmit() (TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE))
// Used to resume a transfer, clear TWINT and ensure that  I2C and interrupts
// are enabled.
#define I2CSendACK()                                                           \
  (TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA))
// FOR MR mode. Resume a transfer, ensure that I2C and  interrupts are enabled
// and respond with an ACK if the  device is addressed as a slave or after it
// receives a byte
#define I2CSendNACK() (TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE))
// FOR MR mode. Resume a transfer, ensure that I2C and // interrupts are enabled
// but DO NOT respond with an ACK  if the device is addressed as a slave or
// after it receives a byte.
typedef enum {
  Ready,
  Initializing,
  RepeatedStartSent,
  MasterTransmitter,
  MasterReceiver,
  SlaceTransmitter,
  SlaveReciever
} I2CMode;

typedef struct I2CInfoStruct {
  I2CMode mode;
  uint8_t errorCode;
  uint8_t repStart;
} I2CInfoStruct;

uint8_t isI2CReady(void);
uint8_t I2CReadData(uint8_t I2Caddr, uint8_t bytesToRead, uint8_t repStart);
uint8_t I2CTransmitData(void *const TXdata, uint8_t dataLen, uint8_t repStart);
uint8_t I2CWriteRegister(uint8_t device, uint8_t address, uint8_t value);
uint8_t I2CWriteRegisterMultiple(uint8_t device, uint8_t address,
                                 uint8_t *value, uint8_t length);
uint8_t I2CReadRegister(uint8_t device, uint8_t address, uint8_t bytesToRead,
                        bool isWii);
I2CInfoStruct I2CInfo;
int RXBuffLen;
int TXBuffLen;
int RXBuffIndex;
volatile int TXBuffIndex;
volatile uint8_t I2CReceiveBuffer[RXMAXBUFLEN];
uint8_t I2CTransmitBuffer[TXMAXBUFLEN];
void i2c_init() {
  I2CInfo.mode = Ready;
  I2CInfo.errorCode = 0xFF;
  I2CInfo.repStart = 0;
  // Set pre-scalers (no pre-scaling)
  TWSR = 0;
  // Set bit rate
  TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;
  // Enable I2C and interrupt
  TWCR = (1 << TWIE) | (1 << TWEN);
  sei();
}

uint8_t isI2CReady() {
  return (I2CInfo.mode == Ready) || (I2CInfo.mode == RepeatedStartSent);
}
uint8_t I2CWriteRegister(uint8_t device, uint8_t addr, uint8_t value) {
  uint8_t msg[] = {(uint8_t)((device << 1) & 0xFE), addr, value};
  I2CTransmitData(msg, 3, 0);
  WAIT_I2C;
  return 1;
}
uint8_t I2CWriteRegisterMultiple(uint8_t device, uint8_t addr, uint8_t *value,
                                 uint8_t bytesToWrite) {
  uint8_t msg[2 + bytesToWrite];
  msg[0] = (device << 1) & 0xFE;
  msg[1] = addr;
  for (int i = 0; i < bytesToWrite; i++) {
    msg[i + 2] = value[i];
  }

  I2CTransmitData(msg, bytesToWrite + 2, 0);
  WAIT_I2C;
  return 1;
}
uint8_t I2CReadRegister(uint8_t device, uint8_t address, uint8_t bytesToRead,
                        bool isWii) {
  uint8_t msg[] = {(uint8_t)((device << 1) & 0xFE), address};
  I2CTransmitData(msg, 2, 0);
  if (isWii) {
    _delay_us(200);
  }
  WAIT_I2C;
  I2CReadData(device, bytesToRead, 0);
  WAIT_I2C;
  return 1;
}
uint8_t I2CTransmitData(void *const TXdata, uint8_t dataLen, uint8_t repStart) {
  if (dataLen <= TXMAXBUFLEN) {
    // Wait until ready
    WAIT_I2C;
    // Set repeated start mode
    I2CInfo.repStart = repStart;
    // Copy data into the transmit buffer
    uint8_t *data = (uint8_t *)TXdata;
    for (int i = 0; i < dataLen; i++) {
      I2CTransmitBuffer[i] = data[i];
    }
    // Copy transmit info to global variables
    TXBuffLen = dataLen;
    TXBuffIndex = 0;

    // If a repeated start has been sent, then devices are already listening for
    // an address and another start does not need to be sent.
    if (I2CInfo.mode == RepeatedStartSent) {
      I2CInfo.mode = Initializing;
      TWDR = I2CTransmitBuffer[TXBuffIndex++]; // Load data to transmit buffer
      I2CSendTransmit();                       // Send the data
    } else // Otherwise, just send the normal start signal to begin
           // transmission.
    {
      I2CInfo.mode = Initializing;
      I2CSendStart();
    }
  } else {
    return 1; // return an error if data length is longer than buffer
  }
  return 0;
}

uint8_t I2CReadData(uint8_t I2Caddr, uint8_t bytesToRead, uint8_t repStart) {
  // Check if number of bytes to read can fit in the RXbuffer
  if (bytesToRead < RXMAXBUFLEN) {
    // Reset buffer index and set RXBuffLen to the number of bytes to read
    RXBuffIndex = 0;
    RXBuffLen = bytesToRead;
    // Create the one value array for the address to be transmitted
    uint8_t TXdata[1];
    // Shift the address and AND a 1 into the read write bit (set to write mode)
    TXdata[0] = (I2Caddr << 1) | 0x01;
    // Use the I2CTransmitData function to initialize the transfer and address
    // the slave
    I2CTransmitData(TXdata, 1, repStart);
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
 * to use default class value in readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data, bool isWii) {
  return readBytes(devAddr, regAddr, 1, data, isWii);
}
/** Read multiple bytes from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off
 * to use default class value in readTimeout)
 * @return Number of bytes read (-1 indicates failure)
 */
int8_t readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                 uint8_t *data, bool isWii) {
  I2CReadRegister(devAddr, regAddr, length, isWii);
  for (int i = 0; i < length; i++) {
    data[i] = I2CReceiveBuffer[i];
  }
  return length;
}
/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
  return writeBytes(devAddr, regAddr, 1, &data);
}
/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length,
                uint8_t *data) {
  return I2CWriteRegisterMultiple(devAddr, regAddr, data, length);
}
ISR(TWI_vect) { 
  switch (I2C_STATUS) {
  // ----\/ ---- MASTER TRANSMITTER OR WRITING ADDRESS ----\/ ----  //
  case I2C_MT_SLAW_ACK: // SLA+W transmitted and ACK received
    // Set mode to Master Transmitter
    I2CInfo.mode = MasterTransmitter;
  case I2C_START_SENT:           // Start condition has been transmitted
  case I2C_MT_DATA_ACK:          // Data byte has been transmitted, ACK received
    if (TXBuffIndex < TXBuffLen) // If there is more data to send
    {
      TWDR = I2CTransmitBuffer[TXBuffIndex++]; // Load data to transmit buffer
      I2CInfo.errorCode = I2C_NO_RELEVANT_INFO;
      I2CSendTransmit(); // Send the data
    }
    // This transmission is complete however do not release bus yet
    else if (I2CInfo.repStart) {
      I2CInfo.errorCode = 0xFF;
      I2CSendStart();
    }
    // All transmissions are complete, exit
    else {
      I2CInfo.mode = Ready;
      I2CInfo.errorCode = 0xFF;
      I2CSendStop();
    }
    break;

    // ----\/ ---- MASTER RECEIVER ----\/ ----  //

  case I2C_MR_SLAR_ACK: // SLA+R has been transmitted, ACK has been received
    // Switch to Master Receiver mode
    I2CInfo.mode = MasterReceiver;
    // If there is more than one byte to be read, receive data byte and return
    // an ACK
    if (RXBuffIndex < RXBuffLen - 1) {
      I2CInfo.errorCode = I2C_NO_RELEVANT_INFO;
      I2CSendACK();
    }
    // Otherwise when a data byte (the only data byte) is received, return NACK
    else {
      I2CInfo.errorCode = I2C_NO_RELEVANT_INFO;
      I2CSendNACK();
    }
    break;

  case I2C_MR_DATA_ACK: // Data has been received, ACK has been transmitted.

    /// -- HANDLE DATA BYTE --- ///
    I2CReceiveBuffer[RXBuffIndex++] = TWDR;
    // If there is more than one byte to be read, receive data byte and return
    // an ACK
    if (RXBuffIndex < RXBuffLen - 1) {
      I2CInfo.errorCode = I2C_NO_RELEVANT_INFO;
      I2CSendACK();
    }
    // Otherwise when a data byte (the only data byte) is received, return NACK
    else {
      I2CInfo.errorCode = I2C_NO_RELEVANT_INFO;
      I2CSendNACK();
    }
    break;

  case I2C_MR_DATA_NACK: // Data byte has been received, NACK has been
                         // transmitted. End of transmission.

    /// -- HANDLE DATA BYTE --- ///
    I2CReceiveBuffer[RXBuffIndex++] = TWDR;
    // This transmission is complete however do not release bus yet
    if (I2CInfo.repStart) {
      I2CInfo.errorCode = 0xFF;
      I2CSendStart();
    }
    // All transmissions are complete, exit
    else {
      I2CInfo.mode = Ready;
      I2CInfo.errorCode = 0xFF;
      I2CSendStop();
    }
    break;

    // ----\/ ---- MT and MR common ----\/ ---- //

  case I2C_MR_SLAR_NACK: // SLA+R transmitted, NACK received
  case I2C_MT_SLAW_NACK: // SLA+W transmitted, NACK received
  case I2C_MT_DATA_NACK: // Data byte has been transmitted, NACK received
  case I2C_LOST_ARBIT:   // Arbitration has been lost
    // Return error and send stop and set mode to ready
    if (I2CInfo.repStart) {
      I2CInfo.errorCode = I2C_STATUS;
      I2CSendStart();
    }
    // All transmissions are complete, exit
    else {
      I2CInfo.mode = Ready;
      I2CInfo.errorCode = I2C_STATUS;
      I2CSendStop();
    }
    break;
  case I2C_REP_START_SENT: // Repeated start has been transmitted
    // Set the mode but DO NOT clear TWINT as the next data is not yet ready
    I2CInfo.mode = RepeatedStartSent;
    break;

  // ----\/ ---- MISCELLANEOUS STATES ----\/ ----  //
  case I2C_NO_RELEVANT_INFO: // It is not really possible to get into this ISR
                             // on this condition Rather, it is there to be
                             // manually set between operations
    break;
  case I2C_ILLEGAL_START_STOP: // Illegal START/STOP, abort and return error
    I2CInfo.errorCode = I2C_ILLEGAL_START_STOP;
    I2CInfo.mode = Ready;
    I2CSendStop();
    break;
  }
 }
