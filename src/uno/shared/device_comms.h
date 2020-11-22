#pragma once
#ifndef UDR1
#  define UBRR1 UBRR0
#  define UDR1 UDR0
#  define UCSR1A UCSR0A
#  define UCSR1B UCSR0B
#  define UCSR1C UCSR0C
#  define UCSZ10 UCSZ00
#  define UCSZ11 UCSZ01
#  define UDRE1 UDRE0
#  define TXC1 TXC0
#  define RXC1 RXC0
#  define TXEN1 TXEN0
#  define RXEN1 RXEN0
#  define RXCIE1 RXCIE0
#  define U2X1 U2X0
#endif
#include "util/util.h"
#include <LUFA/Drivers/Peripheral/Serial.h>
#define BAUD 1000000
#define FRAME_RESET 0x79
#define FRAME_SPLIT 0x7a
#define FRAME_START_DEVICE 0x7c
#define FRAME_START_FEATURE_READ 0x7d
#define FRAME_START_FEATURE_WRITE 0x7e
#define FRAME_START_READ 0x78
#define FRAME_END 0x7f
#define ESC 0x7b
#define FRAME_READY_FOR_REPORT 0x77
static inline void Serial_InitInterrupt(const uint32_t BaudRate,
                                        const bool DoubleSpeed) {
  UBRR1 =
      (DoubleSpeed ? SERIAL_2X_UBBRVAL(BaudRate) : SERIAL_UBBRVAL(BaudRate));

  UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
  UCSR1A = (DoubleSpeed ? (1 << U2X1) : 0);
  UCSR1B = ((1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1));

  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
}

// If we are writing data that has a special purpose, then we write an
// escape byte followed by the escaped data we escape data by xoring with
// 0x20
static inline bool shouldEscape(uint8_t data) {
  return data == FRAME_START_DEVICE || data == FRAME_START_FEATURE_READ ||
         data == FRAME_START_FEATURE_WRITE || data == ESC ||
         data == FRAME_END || data == FRAME_SPLIT || data == FRAME_RESET ||
         data == FRAME_READY_FOR_REPORT || data == FRAME_START_READ;
}


static inline void RingBuffer_Insert_Escaped(RingBuff_t *buf, uint8_t data) {
  if (shouldEscape(data)) {
    RingBuffer_Insert(buf, ESC);
    data ^= 0x20;
  }
  RingBuffer_Insert(buf, data);
}

static inline void Serial_SendByte_Escaped(uint8_t data) {
  if (shouldEscape(data)) {
    Serial_SendByte(ESC);
    data ^= 0x20;
  }
  Serial_SendByte(data);
}