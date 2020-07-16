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

volatile bool done = false;
volatile uint8_t len = 0;
RingBuff_t Receive_Buffer;
uint8_t Receive_BufferData[BUFFER_SIZE];
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

  Serial_InitInterrupt(BAUD, true);

  USB_Init();
  RingBuffer_InitBuffer(&Receive_Buffer, Receive_BufferData);

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
  sei();
  uint8_t byteToWrite;
  uint8_t count = 0;
  while (true) {
    count = RingBuffer_GetCount(&Receive_Buffer);
    if (count >= 2) {
      byteToWrite = RingBuffer_Remove(&Receive_Buffer);
      if (byteToWrite == FRAME_START_DEVICE) {
        // All reports put the first id as the report id.
        uint8_t rid = RingBuffer_Remove(&Receive_Buffer);
        bool writeID = true;
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
          writeID = false;
          break;
        case REPORT_ID_GAMEPAD:
          writeID = false;
        default:
          Endpoint_SelectEndpoint(HID_EPADDR_IN);
          break;
        }
        if (Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady()) {
          /* Read bytes from the USART receive buffer into the USB IN
          endpoint
           */
          if (writeID) { Endpoint_Write_8(rid); }
          bool esc = false;
          while (true) {
            if (count == 0) {
              count = RingBuffer_GetCount(&Receive_Buffer);
              continue;
            }
            count--;
            byteToWrite = RingBuffer_Remove(&Receive_Buffer);
            if (byteToWrite == ESC) {
              esc = true;
              continue;
            } else if (esc) {
              byteToWrite ^= 0x20;
              esc = false;
            } else if (byteToWrite == FRAME_END) {
              break;
            }
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
void processHIDReadFeatureReport(void) {
  done = false;
  len = 0;
  Serial_SendByte(FRAME_START_FEATURE_READ);
  while (!done) {}
  Endpoint_Write_Control_Stream_LE(dbuf, len);
}
bool processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  if (data[0] == COMMAND_REBOOT || data[0] == COMMAND_JUMP_BOOTLOADER) {
    // TODO: For some reason this doesnt always fire, yet the 328p does seem to reboot, indicating that the command is correct but it just isnt rebooting.
    // Would that mean that somehow this if statement isnt being entered, even though the correct data is being written?
    jmpToBootloader = data[0] == COMMAND_REBOOT ? 0 : JUMP;
    reboot();
  }
  if (data[0] == COMMAND_WRITE_SUBTYPE) {
    eeprom_update_byte(&config.deviceType, data[1]);
  }
  Serial_SendByte(FRAME_START_FEATURE_WRITE);
  while (data_len--) { sendData(*(data++)); }
  Serial_SendByte(FRAME_END);
  return false;
}

void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }
uint8_t frame;
bool esc = false;
ISR(USART1_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR1;
  if (ReceivedByte == FRAME_START_DEVICE ||
      ReceivedByte == FRAME_START_FEATURE_READ) {
    frame = ReceivedByte;
  }
  if (frame == FRAME_START_DEVICE) {
    RingBuffer_Insert(&Receive_Buffer, ReceivedByte);
    if (ReceivedByte == FRAME_END) { frame = 0; }
  } else if (frame == FRAME_START_FEATURE_READ) {
    if (ReceivedByte == FRAME_END) {
      frame = 0;
      done = true;
      return;
    }
    if (ReceivedByte == FRAME_START_FEATURE_READ) return;
    if (ReceivedByte == ESC) {
      esc = true;
      return;
    } else if (esc) {
      esc = false;
      ReceivedByte ^= 0x20;
    }
    dbuf[len++] = ReceivedByte;
  }
}