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
size_t controllerIndex = 0;
Controller_t controller;
uint8_t currentReport[sizeof(USB_Report_Data_t)];
uint8_t previousReport[sizeof(USB_Report_Data_t)];
/** Buffers to hold serial data */
RingBuffer_t serialInBuffer;
uint8_t serialInBufferData[128];
RingBuffer_t serialOutBuffer;
uint8_t serialOutBufferData[128];
uint8_t dbuf[DBUF_SIZE];

void writeToSerial(uint8_t data) {
  // If we are writing data that has a special purpose, then we write an escape
  // byte followed by the escaped data we escape data by xoring with 0x20
  if (data == FRAME_START_DEVICE || data == FRAME_START_SERIAL || data == ESC ||
      data == FRAME_END) {
    RingBuffer_Insert(&serialOutBuffer, ESC);
    data = data ^ 0x20;
  }
  RingBuffer_Insert(&serialOutBuffer, data);
}
int main(void) {
  loadConfig();
  UBRR0 = SERIAL_2X_UBBRVAL(1000000);
  UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
  UCSR0A = (1 << U2X0);
  UCSR0B = ((1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0));
  RingBuffer_InitBuffer(&serialInBuffer, serialInBufferData,
                        sizeof(serialInBufferData));
  RingBuffer_InitBuffer(&serialOutBuffer, serialOutBufferData,
                        sizeof(serialOutBufferData));
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
      controllerIndex = 0;
      RingBuffer_Insert(&serialOutBuffer, FRAME_START_DEVICE);
      while (controllerIndex < size) {
        RingBuffer_Insert(&serialOutBuffer, currentReport[controllerIndex++]);
      }
      RingBuffer_Insert(&serialOutBuffer, FRAME_END);
      memcpy(previousReport, currentReport, size);
    }
    size = RingBuffer_GetCount(&serialInBuffer);
    if (size != 0) {
      uint8_t report = RingBuffer_Remove(&serialInBuffer);
      bool isWriting = RingBuffer_Remove(&serialInBuffer);
      if (isWriting) {
        uint8_t len = RingBuffer_Remove(&serialInBuffer);
        for (uint8_t i = 0; i < len; i++) {
          dbuf[i] = RingBuffer_Remove(&serialInBuffer);
        }
        processHIDWriteFeatureReport(report, len, dbuf);
      } else {
        processHIDReadFeatureReport(report);
      }
    }
    size = RingBuffer_GetCount(&serialOutBuffer);
    while (size--) {
      loop_until_bit_is_set(UCSR0A, UDRE0);
      UDR0 = RingBuffer_Remove(&serialOutBuffer);
    }
  }
}
void Endpoint_Write_Control_Stream_LE(const void *const Buffer,
                                      uint16_t Length) {
  RingBuffer_Insert(&serialOutBuffer, FRAME_START_SERIAL);
  uint8_t *buf = (uint8_t *)Buffer;
  while (Length--) { writeToSerial(*(buf++)); }
  RingBuffer_Insert(&serialOutBuffer, FRAME_END);
}
/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
#ifdef USART_RX_vect
ISR(USART_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR0;
  RingBuffer_Insert(&serialInBuffer, ReceivedByte);
}
#else
ISR(USART0_RX_vect, ISR_BLOCK) {
  uint8_t ReceivedByte = UDR0;
  RingBuffer_Insert(&serialInBuffer, ReceivedByte);
}
#endif