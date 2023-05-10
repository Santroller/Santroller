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
    uint8_t packet_id;
} RfHeartbeatPacket_t;

#if DEVICE_TYPE_KEYBOARD
typedef struct {
    uint8_t packet_id;
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
    };
} __attribute__((packed)) RfInputPacket_t;
#else
typedef struct {
    uint8_t packet_id;
    uint8_t transmitter;
    PS3_REPORT lastControllerReport;
} __attribute__((packed)) RfInputPacket_t;
#endif
typedef struct {
    uint8_t packet_id;
    uint8_t consoleType;
} __attribute__((packed)) RfConsoleTypePacket_t;

typedef struct {
    uint8_t packet_id;
} __attribute__((packed)) RfAuthLedPacket_t;

typedef struct {
    uint8_t packet_id;
    uint8_t player;
} __attribute__((packed)) RfPlayerLed_t;
typedef struct {
    uint8_t packet_id;
    uint8_t led;
} __attribute__((packed)) RfKeyboardLed_t;

typedef struct {
    uint8_t packet_id;
    uint8_t left;
    uint8_t right;
} __attribute__((packed)) RfRumbleLed_t;