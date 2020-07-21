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
RingBuff_t Receive_Buffer;
uint8_t Receive_BufferData[BUFFER_SIZE];
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
static inline int readPacket(void) {
  uint8_t len = 0;
  uint8_t data;
  bool escapeNext = false;
  uint8_t count = 0;
  while (true) {
    if (count == 0) {
      count = RingBuffer_GetCount(&Receive_Buffer);
      continue;
    }
    count--;
    data = RingBuffer_Remove(&Receive_Buffer);
    if (data == FRAME_END) {
      break;
    } else if (escapeNext) {
      escapeNext = false;
      data = data ^ 0x20;
    } else if (data == ESC) {
      escapeNext = true;
      continue;
    }
    dbuf[len++] = data;
  }
  return len;
}
int main(void) {
  loadConfig();
  RingBuffer_InitBuffer(&Receive_Buffer, Receive_BufferData);
  Serial_InitInterrupt(BAUD, true);
  sei();
  initInputs();
  initLEDs();
  initReports();
  while (true) {
    tickInputs(&controller);
    tickLEDs(&controller);
    if (!RingBuffer_IsEmpty(&Receive_Buffer)) {
      uint8_t data = RingBuffer_Remove(&Receive_Buffer);
      if (data == FRAME_START_FEATURE_WRITE) {
        if (processHIDWriteFeatureReport(readPacket(), dbuf)) {
          processHIDReadFeatureReport();
        }
      }
    }
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
#ifdef USART0_RX_vect
ISR(USART0_RX_vect, ISR_BLOCK) {
#else
ISR(USART_RX_vect, ISR_BLOCK) {
#endif
  uint8_t ReceivedByte = UDR0;
  RingBuffer_Insert(&Receive_Buffer, ReceivedByte);
}