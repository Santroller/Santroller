#pragma once
#include <stdint.h>
#include "NRFLite.h"
extern NRFLite nrfRadio;
typedef enum {
    Heartbeat,
    Input,
    Mask,
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
    uint8_t radio_id;
} RfHeartbeatPacket_t;
typedef struct {
    RFPacket_t packet_id;
    uint8_t radio_id;
    uint8_t mask[sizeof(combined_report_t)];
} RfMaskPacket_t;
typedef struct {
    RFPacket_t packet_id;
    uint8_t radio_id;
    GipHeader_t header;
    uint8_t mask[sizeof(PS3GHLGuitar_Data_t)];
} RfMaskPacketGHL_t;

typedef struct {
    RFPacket_t packet_id;
    uint8_t radio_id;
    uint8_t data[30];
} RfInputPacket_t;


typedef struct {
    RFPacket_t packet_id;
    uint8_t consoleType;
} RfConsoleTypePacket_t;

typedef struct {
    RFPacket_t packet_id;
} RfAuthLedPacket_t;

typedef struct {
    RFPacket_t packet_id;
    uint8_t player;
} RfPlayerLed_t;
typedef struct {
    RFPacket_t packet_id;
    uint8_t led;
} RfKeyboardLed_t;

typedef struct {
    RFPacket_t packet_id;
    uint8_t left;
    uint8_t right;
} RfRumbleLed_t;