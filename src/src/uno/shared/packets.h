#pragma once
#include <stdint.h>

#define READY 0x65
#define VALID_PACKET 0x14
#define CONTROLLER_DATA_REBOOT_ID 0x73
#define DESCRIPTOR_SERIAL_ID 0x74
#define CONTROLLER_DATA_TRANSMIT_ID 0x75
#define CONTROLLER_DATA_REQUEST_ID 0x76
#define DESCRIPTOR_ID 0x77
#define CONTROL_REQUEST_ID 0x78
#define DEVICE_ID 0x79
#define BAUD 1000000
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)
typedef struct {
    uint8_t magic;
    uint8_t id;
    uint8_t len;
} __attribute__((packed)) packet_header_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} __attribute__((packed)) ret_packet_t;

typedef struct {
    packet_header_t header;
    uint8_t data[];
} __attribute__((packed)) data_transmit_packet_t;

typedef struct {
    packet_header_t header;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed)) descriptor_request_t;


typedef struct {
    packet_header_t header;
    uint8_t bmRequestType;
    uint8_t request;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
    uint8_t data[];
} __attribute__((packed)) control_request_t;