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

volatile bool receivedReport = false;
volatile uint8_t len = 0;
EepromConfig_t EEMEM config;
uint8_t defaultConfig[] = {0xa2, 0xd4, 0x15, 0x00, DEVICE_TYPE};

// if jmpToBootloader is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint32_t jmpToBootloader __attribute__((section(".noinit")));

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
  while (true) { USB_USBTask(); }
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
void processHIDReadFeatureReport(void) {
  receivedReport = false;
  len = 0;
  Serial_SendByte(FRAME_START_FEATURE_READ);
  while (!receivedReport) {}
  Endpoint_Write_Control_Stream_LE(dbuf, len);
}
bool processHIDWriteFeatureReport(uint8_t data_len, uint8_t *data) {
  uint8_t report = data[0];
  uint8_t subType = data[1];
  if (report == COMMAND_WRITE_SUBTYPE) {
    eeprom_update_byte(&config.deviceType, subType);
  }
  if (report == COMMAND_REBOOT || report == COMMAND_JUMP_BOOTLOADER) {
    jmpToBootloader = report == COMMAND_REBOOT ? 0 : JUMP;
    reboot();
  }
  Serial_SendByte(FRAME_START_FEATURE_WRITE);
  while (data_len--) {
    uint8_t d = *(data++);
    if (shouldEscape(d)) {
      Serial_SendByte(ESC);
      d ^= 0x20;
    }
    Serial_SendByte(d);
  }
  Serial_SendByte(FRAME_END);
  return false;
}

void EVENT_USB_Device_ControlRequest(void) { deviceControlRequest(); }
uint8_t frame;
bool escapeNext = false;
bool reportIDNext = false;
const uint8_t endpoints[] = {[REPORT_ID_XINPUT] = XINPUT_EPADDR_IN,
                             [REPORT_ID_XINPUT_2] = XINPUT_2_EPADDR_IN,
                             [REPORT_ID_XINPUT_3] = XINPUT_3_EPADDR_IN,
                             [REPORT_ID_XINPUT_4] = XINPUT_2_EPADDR_IN,
                             [REPORT_ID_GAMEPAD] = HID_EPADDR_IN,
                             [REPORT_ID_KBD] = HID_EPADDR_IN,
                             [REPORT_ID_MOUSE] = HID_EPADDR_IN,
                             [REPORT_ID_MIDI] = MIDI_EPADDR_IN};
ISR(USART1_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR1;
  if (escapeNext) {
    escapeNext = false;
    ReceivedByte ^= 0x20;
  } else if (ReceivedByte == ESC) {
    escapeNext = true;
    return;
  }
  if (ReceivedByte == FRAME_START_DEVICE) { reportIDNext = true; }
  if (ReceivedByte == FRAME_START_DEVICE ||
      ReceivedByte == FRAME_START_FEATURE_READ) {
    frame = ReceivedByte;
    return;
  }
  if (ReceivedByte == FRAME_END) {
    if (frame == FRAME_START_FEATURE_READ) {
      receivedReport = true;
    } else {
      Endpoint_ClearIN();
    }
    frame = 0;
  }
  if (frame == FRAME_START_DEVICE) {
    if (reportIDNext) {
      reportIDNext = false;
      Endpoint_SelectEndpoint(endpoints[ReceivedByte]);
      if (ReceivedByte == REPORT_ID_MIDI || ReceivedByte == REPORT_ID_GAMEPAD) {
        return;
      }
    }
    if (Endpoint_IsReadWriteAllowed() && Endpoint_IsINReady()) {
      Endpoint_Write_8(ReceivedByte);
    }

  } else if (frame == FRAME_START_FEATURE_READ) {
    dbuf[len++] = ReceivedByte;
  }
}