#include "config/eeprom.h"
#include "device_comms.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <stddef.h>
#include <stdlib.h>
#include <util/delay.h>
Controller_t controller;
uint8_t currentReport[sizeof(USB_Report_Data_t)];
uint8_t previousReport[sizeof(USB_Report_Data_t)];
uint8_t dbuf[DBUF_SIZE];
Configuration_t newConfig;
volatile uint8_t reportToHandle = 0;
void writePacketToSerial(uint8_t frame, uint8_t *buf, uint8_t len) {
  Serial_SendByte(frame);
  uint8_t data;
  while (len--) {
    data = *(buf++);
    // If we are writing data that has a special purpose, then we write an
    // escape byte followed by the escaped data we escape data by xoring with
    // 0x20
    if (data == FRAME_START_DEVICE || data == FRAME_START_FEATURE_READ ||
        data == FRAME_START_FEATURE_WRITE || data == ESC || data == FRAME_END) {
      Serial_SendByte(ESC);
      data = data ^ 0x20;
    }
    Serial_SendByte(data);
  }
  Serial_SendByte(FRAME_END);
}
int main(void) {
  loadConfig();
  Serial_InitInterrupt(BAUD, true);
  sei();
  initInputs();
  initReports();
  while (true) {
    if (reportToHandle) {
      if (getData(reportToHandle)) { processHIDReadFeatureReport(); }
      reportToHandle = 0;
    }
    tickInputs(&controller);
    tickLEDs(&controller);
    uint16_t size;
    fillReport(currentReport, &size, &controller);
    if (memcmp(currentReport, previousReport, size) != 0) {
      writePacketToSerial(FRAME_START_DEVICE, currentReport, size);
      memcpy(previousReport, currentReport, size);
    }
  }
}
// Data being written back to USB after a read
void writeToUSB(const void *const Buffer, uint16_t Length) {
  uint8_t *buf = (uint8_t *)Buffer;
  writePacketToSerial(FRAME_START_FEATURE_READ, buf, Length);
}
static uint8_t frame = 0;
bool escapeNext = false;
uint8_t report = 0;
uint8_t *data;
bool isConfig = false;
#ifdef USART0_RX_vect
ISR(USART0_RX_vect, ISR_BLOCK) {
#else
ISR(USART_RX_vect, ISR_BLOCK) {
#endif
  uint8_t ReceivedByte = UDR0;
  if (escapeNext) {
    ReceivedByte ^= 0x20;
    escapeNext = false;
  } else if (ReceivedByte == FRAME_START_FEATURE_WRITE) {
    data = NULL;
    frame = ReceivedByte;
    report = 0;
    isConfig = false;
    return;
  } else if (ReceivedByte == FRAME_END) {
    if (isConfig) {
      eeprom_update_block(&config, &config_pointer, sizeof(config));
    }
    reportToHandle = report;
    frame = 0;
    return;
  } else if (ReceivedByte == ESC) {
    escapeNext = true;
    return;
  }
  if (frame == 0) { return; }
  if (report == 0) {
    report = ReceivedByte;
    if (report == COMMAND_WRITE_CONFIG) {
      data = (uint8_t *)&config;
    } else if (report == COMMAND_SET_LEDS) {
      data = (uint8_t *)&controller.leds;
    }
  } else if (!isConfig && report == COMMAND_WRITE_CONFIG) {
    isConfig = true;
    data += ReceivedByte;
  } else if (data) {
    *(data++) = ReceivedByte;
  }
}