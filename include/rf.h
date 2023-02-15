#pragma once
#include <stdint.h>
#include "NRFLite.h"
extern NRFLite nrfRadio;
typedef enum {
    Heartbeat,
    Input,
    AckConsoleType,
    AckAuthLed,
    AckPlayerLed,
    AckRumble,
    AckKeyboardLed
} RFPacket_t;

typedef enum {
    Wired,
    Wireless
} TransmissionMode_t;

typedef struct {
    RFPacket_t id;
} RfHeartbeatPacket_t;

typedef struct {
    uint8_t id;
    uint8_t response[30];
} RfCommandResponsePacket_t;

typedef struct {
    uint8_t id;
    uint8_t request;
    uint16_t wValue;
} RfCommandRequestPacket_t;

typedef struct {
    RFPacket_t id;
    uint8_t data[30];
} RfInputPacket_t;


typedef struct {
    RFPacket_t id;
    uint8_t consoleType;
} RfConsoleTypePacket_t;

typedef struct {
    RFPacket_t id;
} RfAuthLedPacket_t;

typedef struct {
    RFPacket_t id;
    uint8_t player;
} RfPlayerLed_t;
typedef struct {
    RFPacket_t id;
    uint8_t led;
} RfKeyboardLed_t;

typedef struct {
    RFPacket_t id;
    uint8_t left;
    uint8_t right;
} RfRumbleLed_t;