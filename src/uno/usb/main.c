#include "LightweightRingBuff.h"
#include "bootloader/bootloader.h"
#include "config/defaults.h"
#include "config/defines.h"
#include "device_comms.h"
#include "output/control_requests.h"
#include "output/controller_structs.h"
#include "output/serial_commands.h"
#include "util/util.h"
#include <LUFA/Drivers/Board/Board.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Version.h>
#include <avr/wdt.h>
#define JUMP 0xDEAD8001

typedef struct {
  uint32_t id;
  uint8_t deviceType;
} EepromConfig_t;

/** Circular buffer to hold data from the host before it is sent to the device
 * via the serial port. */
RingBuff_t bufferIn;

/** Circular buffer to hold data from the serial port before it is sent to the
 * host as serial. */
RingBuff_t bufferOutSerial;
/** Circular buffer to hold data from the serial port before it is sent to the
 * host as controller inputs*/
RingBuff_t bufferOutDevice;

RingBuff_Data_t bufferInData[BUFFER_SIZE];
RingBuff_Data_t bufferOutSerialData[BUFFER_SIZE];
RingBuff_Data_t bufferOutDeviceData[BUFFER_SIZE];

bool avrdudeInUse = false;
bool isArdwiino = true;
uint8_t lastCommand = 0;
bool waitingForCommandCompletion = false;
EepromConfig_t EEMEM config;
uint8_t defaultConfig[] = {0xa2, 0xd4, 0x15, 0x00, DEVICE_TYPE};

// if jmpToBootloader is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint32_t jmpToBootloader __attribute__((section(".noinit")));

/**
 * Write data from a buffer to an endpoint, handling escape bytes
 */
void writeToEndpoint(uint8_t endpoint, RingBuff_t *buffer,
                     bool isSerialEndpoint);

/**
 * Set the device mode, true for ardwiino, false for avrdude
 */
void setDeviceMode(bool ardwiinoMode) {
  isArdwiino = ardwiinoMode;
  // Disable serial before configuring
  UCSR1B = 0;
  UCSR1A = 0;
  UCSR1C = 0;
  // Set the baudrate. Ardwiino runs at a faster baudrate so that hid is more
  // responsive, but avrdude requires 115200.
  UBRR1 = ardwiinoMode ? SERIAL_2X_UBBRVAL(BAUD) : SERIAL_2X_UBBRVAL(115200);

  // Enable serial again, making sure to enable receive interrupts
  UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
  UCSR1A = (1 << U2X1);
  UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
}

int main(void) {
  // jump to the bootloader at address 0x1000 if jmpToBootloader is set to JUMP
  if (jmpToBootloader == JUMP) {
    // We don't want to jump again after the bootloader returns control flow to
    // us
    jmpToBootloader = 0;
    asm volatile("jmp 0x1000");
  }

  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();

  // Configure serial for ardwiino mode
  setDeviceMode(true);

  /* Start the flush timer so that overflows occur rapidly to push received
   * bytes to the serial interface */
  TCCR0B = (1 << CS02);

  USB_Init();

  /* Start the 328p  */
  AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;

  // Read the device type from eeprom. ARDWIINO_DEVICE_TYPE is used as a
  // signature to make sure that the data in eeprom is valid
  //  if (eeprom_read_dword(&config.id) != ARDWIINO_DEVICE_TYPE) {
  //   eeprom_update_dword(&config.id, ARDWIINO_DEVICE_TYPE);
  //   eeprom_update_byte(&config.deviceType, deviceType);
  // } else {
  //   deviceType = eeprom_read_byte(&config.deviceType);
  // }
  // This is equivilant to the above code, but saves like 80 bytes
  bool deviceIDIsCorrect = true;
  for (uint16_t i = 0; i < sizeof(EepromConfig_t); i++) {
    uint8_t read = eeprom_read_byte(((uint8_t *)&config) + i);
    uint8_t def = defaultConfig[i];
    bool readingDeviceType = i == offsetof(EepromConfig_t, deviceType);
    if ((deviceIDIsCorrect || !readingDeviceType) && read != def) {
      eeprom_write_byte((uint8_t *)i, def);
      deviceIDIsCorrect = false;
    }
    if (readingDeviceType && !deviceIDIsCorrect) { deviceType = read; }
  }

  RingBuffer_InitBuffer(&bufferIn, bufferInData);
  RingBuffer_InitBuffer(&bufferOutSerial, bufferOutSerialData);
  RingBuffer_InitBuffer(&bufferOutDevice, bufferOutDeviceData);
  sei();
  while (true) {
    Endpoint_SelectEndpoint(ENDPOINT_CONTROLEP);
    if (Endpoint_IsSETUPReceived()) USB_Device_ProcessControlRequest();

    if (USB_DeviceState != DEVICE_STATE_Configured) { continue; }
    // We can only read bytes if the buffer isn't full
    if (!(RingBuffer_IsFull(&bufferIn))) {
      Endpoint_SelectEndpoint(CDC_RX_EPADDR);
      if (Endpoint_IsOUTReceived()) {
        if (Endpoint_BytesInEndpoint()) {
          uint8_t receivedByte = Endpoint_Read_8();
          RingBuffer_Insert(&bufferIn, receivedByte);
          if (!(Endpoint_BytesInEndpoint())) Endpoint_ClearOUT();
          // If we are in avrdude mode, then we just care if avrdude has
          // finished or not
          if (!isArdwiino) {
            avrdudeInUse |= receivedByte == COMMAND_STK_500_ENTER_PROG;
          } else if (!waitingForCommandCompletion) {
            // Otherwise, we can parse the bytes as commands. However, we only
            // want to parse full commands. To do this, we set
            // waitingForCommandCompletion once the usb code has parsed a
            // command it recognises.
            // However, there are commands that only the main mcu recognises. It
            // responses to every command however, so we can set
            // waitingForCommandCompletion once we get a response.
            if (lastCommand == COMMAND_WRITE_CONFIG_VALUE) {
              // We only understand Writing to CONFIG_SUB_TYPE, we need to
              // ignore any other command.
              if (receivedByte != CONFIG_SUB_TYPE) {
                waitingForCommandCompletion = true;
                lastCommand = 0;
              } else {
                lastCommand = receivedByte;
              }
            } else if (lastCommand == CONFIG_SUB_TYPE) {
              // We previously received that the section being updated is the
              // subtype, so the current byte is the subtype. Write the new
              // subtype to eeprom.
              eeprom_update_byte(&config.deviceType, receivedByte);
              lastCommand = 0;
              waitingForCommandCompletion = true;
            } else {
              switch (receivedByte) {
                // To save flash, the below commands are written like this.
                // These commands handle either rebooting normally, or jumping
                // to the bootloader.
              case COMMAND_REBOOT:
              case COMMAND_JUMP_BOOTLOADER_UNO:
                jmpToBootloader = receivedByte == COMMAND_REBOOT ? 0 : JUMP;
                reboot();
                // jump_bootloader just sets us to avrdude mode.
              case COMMAND_JUMP_BOOTLOADER:
                setDeviceMode(false);
                break;
              case COMMAND_WRITE_CONFIG_VALUE:
                // We have received a command write config value command, so we
                // save it so we can receive more data next.
                lastCommand = receivedByte;
                break;
              default:
                // We received an unknown command, so wait for the next command.
                waitingForCommandCompletion = true;
              }
            }
          }
        }
      }
    }
    // Write data from the different output buffers to their respective
    // endpoints
    writeToEndpoint(DEVICE_EPADDR_IN, &bufferOutDevice, false);
    writeToEndpoint(CDC_TX_EPADDR, &bufferOutSerial, true);
    // Send the next byte from the input buffer to the  main mcu
    if (!(RingBuffer_IsEmpty(&bufferIn))) {
      Serial_SendByte(RingBuffer_Remove(&bufferIn));
    }
  }
}
void writeToEndpoint(uint8_t endpoint, RingBuff_t *buffer,
                     bool isSerialEndpoint) {

  RingBuff_Count_t bytesToWrite = RingBuffer_GetCount(buffer);
  uint8_t byteToWrite;
  if (bytesToWrite == 0) return;
  if (isSerialEndpoint) {
    // If the uno is sending back data, then that means the command was parsed
    // and it is ready for another command
    waitingForCommandCompletion = false;
    // Serial needs to be buffered a little, this is done by checking if we are
    // about to overflow, or if the flush timer has overflowed.
    if ((((TIFR0 & (1 << TOV0)) == 0) && (bytesToWrite < BUFFER_NEARLY_FULL))) {
      return;
    }
    TIFR0 |= (1 << TOV0);
  }
  if (isArdwiino) {
    byteToWrite = RingBuffer_Remove(buffer);
    if (byteToWrite != FRAME_START_DEVICE &&
        byteToWrite != FRAME_START_SERIAL) {
      return;
    }
    // A device packet should always be smaller than the largest packet. We also
    // need to account for escape bytes however
    bytesToWrite = sizeof(USB_Report_Data_t) + 10;
  }
  Endpoint_SelectEndpoint(endpoint);

  if (Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady()) {
    /* Read bytes from the USART receive buffer into the USB IN
    endpoint
     */
    while (bytesToWrite--) {
      byteToWrite = RingBuffer_Remove(buffer);
      if (isArdwiino) {
        if (byteToWrite == FRAME_END) { break; };
        if (byteToWrite == ESC) byteToWrite = RingBuffer_Remove(buffer) ^ 0x20;
      }
      Endpoint_Write_8(byteToWrite);
    }
    Endpoint_ClearIN();
  }
}

void EVENT_USB_Device_ConfigurationChanged(void) {
  // Setup necessary endpoints
  Endpoint_ConfigureEndpoint(CDC_NOTIFICATION_EPADDR, EP_TYPE_INTERRUPT,
                             CDC_NOTIFICATION_EPSIZE, 1);

  Endpoint_ConfigureEndpoint(CDC_TX_EPADDR, EP_TYPE_BULK, CDC_TX_EPSIZE,
                             CDC_TX_BANK_SIZE);

  Endpoint_ConfigureEndpoint(CDC_RX_EPADDR, EP_TYPE_BULK, CDC_RX_EPSIZE,
                             CDC_RX_BANK_SIZE);

  Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
}
void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }

uint8_t frame = 0;
/** Receive data from the main mcu, and put it into the correct output buffer
 * based on the last known frame
 */
ISR(USART1_RX_vect, ISR_BLOCK) {
  uint8_t receivedByte = UDR1;

  if (receivedByte == FRAME_START_DEVICE ||
      receivedByte == FRAME_START_SERIAL) {
    frame = receivedByte;
  }
  if (frame == FRAME_START_SERIAL || !isArdwiino) {
    RingBuffer_Insert(&bufferOutSerial, receivedByte);
  } else if (frame == FRAME_START_DEVICE) {
    RingBuffer_Insert(&bufferOutDevice, receivedByte);
  }
  if (receivedByte == FRAME_END) { frame = 0; }
}