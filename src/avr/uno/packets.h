#pragma once
#include <stdint.h>
#include "usb.h"
#define READY 0x75
#define VALID_PACKET 0x76
#define DESCRIPTOR_ID 0x77
#define CONTROL_REQUEST_ID 0x78
#define BAUD 1000000
#define DESC_REQUEST_HEADER {VALID_PACKET, DESCRIPTOR_ID, sizeof(descriptor_request_t)}
typedef struct ATTR_PACKED {
    uint8_t magic;
    uint8_t id;
    uint16_t len;
} packet_header_t;

typedef struct ATTR_PACKED {
    packet_header_t header;
    uint16_t data[];
} ret_packet_t;

typedef struct {
    packet_header_t header;
    uint16_t wValue;
    uint16_t wIndex;
} descriptor_request_t;


typedef struct {
    packet_header_t header;
    requestType_t requestType;
    uint8_t request;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    uint8_t data[];
} control_request_t;

typedef struct {
    packet_header_t header;
    
} controller_data_t;

static inline void Serial_InitInterrupt(const uint32_t BaudRate,
                                        const bool DoubleSpeed) {
  UBRR1 = (DoubleSpeed ? SERIAL_2X_UBBRVAL(BaudRate) : SERIAL_UBBRVAL(BaudRate));

  UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
  UCSR1A = (DoubleSpeed ? (1 << U2X1) : 0);
  UCSR1B = ((1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1));

  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
}