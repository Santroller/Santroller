#include "LightweightRingBuff.h"
#include "bootloader/bootloader.h"
#include "config/config.h"
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

/** Circular buffer to hold data from the serial port before it is sent to the
 * host as controller inputs*/
RingBuff_t bufferOutDevice;

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
  RingBuffer_InitBuffer(&bufferOutDevice, bufferOutDeviceData);
  sei();
  while (true) {
    RingBuff_Count_t bytesToWrite = RingBuffer_GetCount(&bufferOutDevice);
    uint8_t byteToWrite;
    if (bytesToWrite != 0) {
      byteToWrite = RingBuffer_Remove(&bufferOutDevice);
      if (byteToWrite == FRAME_START_DEVICE) {
        // All reports put the first id as the report id.
        uint8_t rid = RingBuffer_Peek(&bufferOutDevice);
        switch (rid) {
        case REPORT_ID_XINPUT:
          Endpoint_SelectEndpoint(XINPUT_EPADDR_IN);
          break;
        case REPORT_ID_XINPUT_2:
          Endpoint_SelectEndpoint(XINPUT_2_EPADDR_IN);
          break;
        case REPORT_ID_XINPUT_3:
          Endpoint_SelectEndpoint(XINPUT_3_EPADDR_IN);
          break;
        case REPORT_ID_XINPUT_4:
          Endpoint_SelectEndpoint(XINPUT_4_EPADDR_IN);
          break;
        case REPORT_ID_MIDI:
          Endpoint_SelectEndpoint(MIDI_EPADDR_IN);
          // The "reportid" is actually not a real thing on midi, so we need to
          // strip it before we send data.
          RingBuffer_Remove(&bufferOutDevice);
          break;
        default:
          Endpoint_SelectEndpoint(HID_EPADDR_IN);
          // Wii RB Guitars don't know what to do with report ids, so we skip it
          // here. This does mean that the guitar wont work on a pc, but what
          // else are we gonna do
          if (deviceType == WII_ROCK_BAND_GUITAR) {
            RingBuffer_Remove(&bufferOutDevice);
          }
          break;
        }
        if (Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady()) {
          /* Read bytes from the USART receive buffer into the USB IN
          endpoint
           */
          while (true) {
            if (bytesToWrite-- == 0) {
              bytesToWrite = RingBuffer_GetCount(&bufferOutDevice);
              continue;
            }
            byteToWrite = RingBuffer_Remove(&bufferOutDevice);
            if (byteToWrite == FRAME_END) { break; };
            if (byteToWrite == ESC)
              byteToWrite = RingBuffer_Remove(&bufferOutDevice) ^ 0x20;
            Endpoint_Write_8(byteToWrite);
          }
          Endpoint_ClearIN();
        }
      }
    }
    USB_USBTask();
  }
}
void sendData(uint8_t data) {
  if (data == FRAME_START_FEATURE_READ || data == FRAME_START_FEATURE_WRITE ||
      data == FRAME_END || data == ESC) {
    Serial_SendByte(ESC);
    data = data ^ 0x20;
  }
  Serial_SendByte(data);
}
void EVENT_USB_Device_ConfigurationChanged(void) {
  // Setup necessary endpoints
  Endpoint_ConfigureEndpoint(XINPUT_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
#ifdef MULTI_ADAPTOR
  Endpoint_ConfigureEndpoint(XINPUT_2_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(XINPUT_3_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
  Endpoint_ConfigureEndpoint(XINPUT_4_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE,
                             1);
#else
  Endpoint_ConfigureEndpoint(HID_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
  Endpoint_ConfigureEndpoint(MIDI_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE, 1);
#endif
}
void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }
void processHIDReadFeatureReport(void) {
  Serial_SendByte(FRAME_START_FEATURE_READ);
  while (true) {
    if (RingBuffer_IsEmpty(&bufferOutDevice)) { continue; }
    if (RingBuffer_Remove(&bufferOutDevice) == FRAME_START_FEATURE_READ) {
      break;
    }
  }
  uint8_t len = 0;
  uint8_t data;
  bool esc = false;
  while (true) {
    if (RingBuffer_IsEmpty(&bufferOutDevice)) { continue; }
    data = RingBuffer_Remove(&bufferOutDevice);
    if (data == FRAME_END) { break; }
    if (data == ESC) {
      esc = true;
      continue;
    }
    if (esc) {
      esc = false;
      data = data ^ 0x20;
    }
    dbuf[len++] = data;
  }

  Endpoint_Write_Control_Stream_LE(dbuf, len);
}
void processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t* data2 = data;
  Serial_SendByte(FRAME_START_FEATURE_WRITE);
  while (data_len--) { sendData(*(data++)); }
  Serial_SendByte(FRAME_END);
  uint8_t report = *(data2++);
  switch (report) {
  case COMMAND_REBOOT:
  case COMMAND_JUMP_BOOTLOADER:
    jmpToBootloader = report == COMMAND_REBOOT ? 0 : JUMP;
    reboot();
    break;
  case COMMAND_WRITE_SUBTYPE: {
    eeprom_update_byte(&config.deviceType, *data2);
    break;
  }
  }
}

/** Receive data from the main mcu, and put it into the correct output buffer
 * based on the last known frame
 */
ISR(USART1_RX_vect, ISR_BLOCK) { RingBuffer_Insert(&bufferOutDevice, UDR1); }