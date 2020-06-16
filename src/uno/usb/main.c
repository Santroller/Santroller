#include "LightweightRingBuff.h"
#include "bootloader/bootloader.h"
#include "config/defaults.h"
#include "config/defines.h"
#include "device_comms.h"
#include "output/control_requests.h"
#include "output/controller_structs.h"
#include "output/serial_commands.h"
#include "config/config.h"
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

EepromConfig_t EEMEM config;
uint8_t defaultConfig[] = {0xa2, 0xd4, 0x15, 0x00, DEVICE_TYPE};

// if jmpToBootloader is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint32_t jmpToBootloader __attribute__((section(".noinit")));

/**
 * Write data from a buffer to an endpoint, handling escape bytes
 */
void writeToEndpoint(uint8_t endpoint, RingBuff_t *buffer);

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

  // Set the baudrate. Ardwiino runs at a faster baudrate so that hid is more
  // responsive, but avrdude requires 115200.
  UBRR1 = SERIAL_2X_UBBRVAL(BAUD);

  // Enable serial again, making sure to enable receive interrupts
  UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
  UCSR1A = (1 << U2X1);
  UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

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
    // Write data from the different output buffers to their respective
    // endpoints
    writeToEndpoint(HID_EPADDR_IN, &bufferOutDevice);
    // Send the next byte from the input buffer to the  main mcu
    if (!(RingBuffer_IsEmpty(&bufferIn))) {
      Serial_SendByte(RingBuffer_Remove(&bufferIn));
    }
  }
}
void writeToEndpoint(uint8_t endpoint, RingBuff_t *buffer) {

  RingBuff_Count_t bytesToWrite = RingBuffer_GetCount(buffer);
  uint8_t byteToWrite;
  if (bytesToWrite == 0) return;
  byteToWrite = RingBuffer_Remove(buffer);
  if (byteToWrite != FRAME_START_DEVICE && byteToWrite != FRAME_START_SERIAL) {
    return;
  }
  // A device packet should always be smaller than the largest packet. We also
  // need to account for escape bytes however
  bytesToWrite = sizeof(USB_Report_Data_t) + 10;
  Endpoint_SelectEndpoint(endpoint);

  if (Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady()) {
    /* Read bytes from the USART receive buffer into the USB IN
    endpoint
     */
    while (bytesToWrite--) {
      byteToWrite = RingBuffer_Remove(buffer);
      if (byteToWrite == FRAME_END) { break; };
      if (byteToWrite == ESC) byteToWrite = RingBuffer_Remove(buffer) ^ 0x20;
      Endpoint_Write_8(byteToWrite);
    }
    Endpoint_ClearIN();
  }
}

void EVENT_USB_Device_ConfigurationChanged(void) {
  // Setup necessary endpoints

  Endpoint_ConfigureEndpoint(XINPUT_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(HID_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(MIDI_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
}
void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }
extern uint8_t dbuf[sizeof(USB_Descriptor_Configuration_t)];
void processHIDReadFeatureReport(uint8_t report) {
  uint8_t len = 0;
  Serial_SendByte(report);
  Serial_SendByte(false);
  while (true) {
    if (RingBuffer_IsEmpty(&bufferOutSerial)) { continue; }
    uint8_t data = RingBuffer_Remove(&bufferOutSerial);
    if (data == FRAME_START_SERIAL) { continue; }
    if (data == FRAME_END) { break; }
    dbuf[len++] = data;
  }
  Endpoint_Write_Control_Stream_LE(dbuf, len);
}
void processHIDWriteFeatureReport(uint8_t report, uint8_t data_len,
                                  uint8_t *data) {
  switch (report) {
  case COMMAND_REBOOT:
  case COMMAND_JUMP_BOOTLOADER_UNO:
    jmpToBootloader = report == COMMAND_REBOOT ? 0 : JUMP;
    reboot();
    break;
  case COMMAND_CONFIG:
    eeprom_update_byte(&config.deviceType, data[offsetof(Configuration_t,main.subType)]);
    break;
  }
  RingBuffer_Insert(&bufferIn, report);
  RingBuffer_Insert(&bufferIn, true);
  RingBuffer_Insert(&bufferIn, data_len);
  while (data_len--) { RingBuffer_Insert(&bufferIn, *(data++)); }
}

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
  if (frame == FRAME_START_SERIAL) {
    RingBuffer_Insert(&bufferOutSerial, receivedByte);
  } else if (frame == FRAME_START_DEVICE) {
    RingBuffer_Insert(&bufferOutDevice, receivedByte);
  }
  if (receivedByte == FRAME_END) { frame = 0; }
}