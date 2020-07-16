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
#include <LightweightRingBuff.h>
#define BAUD 1000000
#define FRAME_START_DEVICE 0x7c
#define FRAME_START_FEATURE_READ 0x7d
#define FRAME_START_FEATURE_WRITE 0x7e
#define FRAME_END 0x7f
#define ESC 0x7b
extern RingBuff_t Receive_Buffer;
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

static inline int readData(void) {
  uint8_t len = 0;
  uint8_t data;
  bool esc = false;
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
    } else if (esc) {
      esc = false;
      data = data ^ 0x20;
    } else if (data == ESC) {
      esc = true;
      continue;
    }
    dbuf[len++] = data;
  }
  return len;
}