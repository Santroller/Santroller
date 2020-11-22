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

const uint8_t endpoints[] = {[REPORT_ID_CONTROL] = ENDPOINT_CONTROLEP,
                             [REPORT_ID_XINPUT] = XINPUT_EPADDR_IN,
                             [REPORT_ID_XINPUT_2] = XINPUT_2_EPADDR_IN,
                             [REPORT_ID_XINPUT_3] = XINPUT_3_EPADDR_IN,
                             [REPORT_ID_XINPUT_4] = XINPUT_2_EPADDR_IN,
                             [REPORT_ID_GAMEPAD] = HID_EPADDR_IN,
                             [REPORT_ID_KBD] = HID_EPADDR_IN,
                             [REPORT_ID_MOUSE] = HID_EPADDR_IN,
                             [REPORT_ID_MIDI] = MIDI_EPADDR_IN};
typedef struct {
  uint32_t id;
  uint8_t deviceType;
} EepromConfig_t;

EepromConfig_t EEMEM config;
uint8_t defaultConfig[] = {0xa2, 0xd4, 0x15, 0x00, DEVICE_TYPE};

// if jmpToBootloader is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint32_t jmpToBootloader __attribute__((section(".noinit")));

uint8_t frame;
RingBuff_Data_t inBufData[BUFFER_SIZE];
RingBuff_Data_t outBufData[BUFFER_SIZE];
RingBuff_t inBuf;
RingBuff_t outBuf;
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
  RingBuffer_InitBuffer(&inBuf, inBufData);
  RingBuffer_InitBuffer(&outBuf, outBufData);

  /* Start the 328p  */
  AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
  AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;

  // Read the device type from eeprom. ARDWIINO_DEVICE_TYPE is used as a
  // signature to make sure that the data in eeprom is valid
  if (eeprom_read_dword(&config.id) != ARDWIINO_DEVICE_TYPE) {
    eeprom_update_dword(&config.id, ARDWIINO_DEVICE_TYPE);
    eeprom_update_byte(&config.deviceType, deviceType);
  } else {
    deviceType = eeprom_read_byte(&config.deviceType);
  }

  sei();
  bool waitingForReportWritten = false;
  bool escapeNext = false;
  bool readEndpoint = false;
  bool reading = false;
  int count;
  uint8_t received;
  while (true) {
    count = RingBuffer_GetCount(&outBuf);
    while (count--) {
      received = RingBuffer_Remove(&outBuf);
      if (received == ESC) {
        escapeNext = true;
        continue;
      } else if (escapeNext) {
        received ^= 0x20;
        escapeNext = false;
      } else if (received == FRAME_START_READ) {
        readEndpoint = true;
      } else if (received == FRAME_END) {
        Endpoint_ClearIN();
        reading = false;
        if (Endpoint_GetCurrentEndpoint() != ENDPOINT_CONTROLEP) {
          waitingForReportWritten = true;
        }
      } else if (readEndpoint) {
        reading = true;
        readEndpoint = false;
        Endpoint_SelectEndpoint(endpoints[received]);
        if (received == REPORT_ID_MIDI || received == REPORT_ID_GAMEPAD) {
          continue;
        }
      }
      if (reading) { Endpoint_Write_8(received); }
    }
    count = RingBuffer_GetCount(&inBuf);
    while (count--) { Serial_SendByte(RingBuffer_Remove(&inBuf)); }
    if (waitingForReportWritten && Endpoint_IsINReady()) {
      RingBuffer_Insert(&inBuf, FRAME_READY_FOR_REPORT);
      waitingForReportWritten = false;
    }
    USB_USBTask();
  }
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
const uint8_t PROGMEM id[] = {0x21, 0x26, 0x01, 0x07, 0x00, 0x00, 0x00, 0x00};
void processHIDReadFeatureReport(uint8_t cmd) {
  Endpoint_ClearSETUP();
  RingBuffer_Insert(&inBuf, FRAME_START_FEATURE_READ);
  RingBuffer_Insert(&inBuf, cmd);
}
void processHIDWriteFeatureReport(uint8_t cmd, uint8_t data_len,
                                  uint8_t *data) {
  uint8_t subType = data[0];
  if (cmd == COMMAND_WRITE_SUBTYPE) {
    eeprom_update_byte(&config.deviceType, subType);
  }
  if (cmd == COMMAND_REBOOT || cmd == COMMAND_JUMP_BOOTLOADER) {
    jmpToBootloader = cmd == COMMAND_REBOOT ? 0 : JUMP;
    reboot();
  }
  RingBuffer_Insert(&inBuf, FRAME_START_FEATURE_WRITE);
  RingBuffer_Insert_Escaped(&inBuf, cmd);
  while (data_len--) { RingBuffer_Insert_Escaped(&inBuf, *(data++)); }
  RingBuffer_Insert(&inBuf, FRAME_END);
}

void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }
ISR(USART1_RX_vect, ISR_BLOCK) { RingBuffer_Insert(&outBuf, UDR1); }