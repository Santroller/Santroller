#pragma once
#include <stdint.h>

#include "RF24.h"
extern RF24 radio;
typedef enum {
    Heartbeat,
    Input,
    ConsoleType,
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

#if SUPPORTS_KEYBOARD
typedef struct {
    RFPacket_t packet_id;
    union {
#ifdef TICK_NKRO
        USB_NKRO_Data_t lastNKROReport;
#endif
#ifdef TICK_MOUSE
        USB_Mouse_Data_t lastMouseReport;
#endif
#ifdef TICK_CONSUMER
        USB_ConsumerControl_Data_t lastMouseReport;
#endif
    }
} RfInputPacket_t;
#else
typedef struct {
    RFPacket_t packet_id;
    PS3_REPORT lastControllerReport;
} RfInputPacket_t;
#endif
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