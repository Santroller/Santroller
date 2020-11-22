#include "LightweightRingBuff.h"
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
RingBuff_Data_t inBufData[BUFFER_SIZE];
RingBuff_Data_t outBufData[BUFFER_SIZE];
RingBuff_t inBuf;
RingBuff_t outBuf;
Configuration_t newConfig;
bool readyForReport = true;
long lastPoll = 0;
void writePacketToSerialBuffer(uint8_t frame, uint8_t *buf, uint8_t len) {
  RingBuffer_Insert(&outBuf, frame);
  while (len--) { RingBuffer_Insert_Escaped(&outBuf, *(buf++)); }
  RingBuffer_Insert(&outBuf, FRAME_END);
}
int main(void) {
  loadConfig();
  Serial_InitInterrupt(BAUD, true);
  sei();
  RingBuffer_InitBuffer(&inBuf, inBufData);
  RingBuffer_InitBuffer(&outBuf, outBufData);
  initInputs();
  initReports();
  bool escapeNext = false;
  int count;
  uint8_t received;
  uint8_t frame = 0;
  uint8_t cmd = 0;
  uint8_t *data = NULL;
  bool waitingForConfigOffset = false;
  while (true) {
    count = RingBuffer_GetCount(&inBuf);
    while (count--) {
      received = RingBuffer_Remove(&inBuf);
      if (escapeNext) {
        received ^= 0x20;
        escapeNext = false;
      } else if (received == FRAME_ESC) {
        escapeNext = true;
        continue;
      } else if (received == FRAME_READY_FOR_REPORT) {
        readyForReport = true;
        continue;
      } else if (received == FRAME_START_FEATURE_WRITE ||
                 received == FRAME_START_FEATURE_READ) {
        frame = received;
        continue;
      } else if (received == FRAME_END) {
        if (cmd == COMMAND_WRITE_CONFIG) {
          eeprom_update_block(&config, &config_pointer, sizeof(config));
        }
        handleCommand(cmd);
        frame = 0;
        cmd = 0;
        data = NULL;
      }
      if (frame != 0) {
        if (frame == FRAME_START_FEATURE_READ) {
          processHIDReadFeatureReport(received);
          frame = 0;
        } else if (cmd == 0) {
          cmd = received;
          if (cmd == COMMAND_WRITE_CONFIG) {
            data = (uint8_t *)&config;
            waitingForConfigOffset = true;
          } else if (cmd == COMMAND_SET_LEDS) {
            data = (uint8_t *)&controller.leds;
          }
        } else if (waitingForConfigOffset) {
          waitingForConfigOffset = false;
          data += received;
        } else if (data) {
          *(data++) = received;
        }
      }
    }
    count = RingBuffer_GetCount(&outBuf);
    while (count--) { Serial_SendByte(RingBuffer_Remove(&outBuf)); }
    tickInputs(&controller);
    tickLEDs(&controller);
    uint16_t size;
    fillReport(currentReport, &size, &controller);
    if (millis() - lastPoll > config.main.pollRate && readyForReport) {
      lastPoll = millis();
      if (memcmp(currentReport, previousReport, size) != 0) {
        readyForReport = false;
        writePacketToSerialBuffer(FRAME_START_WRITE, currentReport, size);
        memcpy(previousReport, currentReport, size);
      }
    }
  }
}
// Data being written back to USB after a read
void writeToUSB(const void *const Buffer, uint16_t Length) {
  uint8_t *buf = (uint8_t *)Buffer;
  writePacketToSerialBuffer(FRAME_START_WRITE, buf, Length);
}
#ifdef USART0_RX_vect
ISR(USART0_RX_vect, ISR_BLOCK) {
#else
ISR(USART_RX_vect, ISR_BLOCK) {
#endif
  RingBuffer_Insert(&inBuf, UDR1);
}