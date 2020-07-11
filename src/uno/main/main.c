#include "config/eeprom.h"
#include "device_comms.h"
#include "input/input_handler.h"
#include "input/inputs/direct.h"
#include "leds/leds.h"
#include "output/reports.h"
#include "output/serial_commands.h"
#include "output/serial_handler.h"
#include "util/util.h"
#include <LUFA/Drivers/Misc/RingBuffer.h>
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
/** Buffers to hold serial data */
RingBuffer_t bufferIn;
uint8_t bufferInData[128];
uint8_t dbuf[DBUF_SIZE];
void writeToSerial(uint8_t data) {
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = data;
}
void writePacketToSerial(uint8_t frame, uint8_t* buf, uint8_t len) {
  writeToSerial(frame);
  uint8_t data;
  while (len--) {
    data = *(buf++);
    // If we are writing data that has a special purpose, then we write an escape
    // byte followed by the escaped data we escape data by xoring with 0x20
    if (data == FRAME_START_DEVICE || data == FRAME_START_FEATURE_READ ||
        data == FRAME_START_FEATURE_WRITE || data == ESC || data == FRAME_END) {
      writeToSerial(ESC);
      data = data ^ 0x20;
    }
    writeToSerial(data);
  }
  writeToSerial(FRAME_END);
}
int main(void) {
  loadConfig();
  UBRR0 = SERIAL_2X_UBBRVAL(1000000);
  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (1 << U2X0);
  UCSR0B = ((1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0));
  RingBuffer_InitBuffer(&bufferIn, bufferInData, sizeof(bufferInData));
  sei();
  initInputs();
  initLEDs();
  initReports();
  while (1) {
    tickInputs(&controller);
    tickLEDs(&controller);
    uint16_t size;
    fillReport(currentReport, &size, &controller);
    if (memcmp(currentReport, previousReport, size) != 0) {
      writePacketToSerial(FRAME_START_DEVICE, currentReport, sizeof(currentReport));
      memcpy(previousReport, currentReport, size);
    }
    if (!RingBuffer_IsEmpty(&bufferIn)) {
      uint8_t packet = RingBuffer_Remove(&bufferIn);
      if (packet == FRAME_START_FEATURE_WRITE) {
        uint8_t len = 0;
        uint8_t data;
        bool esc = false;
        while (true) {
          if (RingBuffer_IsEmpty(&bufferIn)) { continue; }
          data = RingBuffer_Remove(&bufferIn);
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
        processHIDWriteFeatureReport(len, dbuf);
      } else if (packet == FRAME_START_FEATURE_READ) {
        processHIDReadFeatureReport();
      }
    }
  }
}
// Data being written back to USB after a read
void writeToUSB(const void *const Buffer, uint16_t Length) {
  uint8_t *buf = (uint8_t *)Buffer;
  writePacketToSerial(FRAME_START_FEATURE_READ, buf, Length);
}
/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
#ifdef USART_RX_vect
ISR(USART_RX_vect, ISR_BLOCK) {
#else
ISR(USART0_RX_vect, ISR_BLOCK) {
#endif
  uint8_t receivedByte = UDR0;
  RingBuffer_Insert(&bufferIn, receivedByte);
}