
#pragma once
#include <stdint.h>
#include <tusb.h>
#define USB_DP_PIN 20
#define USB_DM_PIN 21
#define USB_FIRST_PIN 20
#include "constants.h"

void init_usb_host(void);
void tick_usb_host(void);
bool send_control_request(uint8_t address, uint8_t endpoint, const tusb_control_request_t request, bool terminateEarly, uint8_t* d);
TUSB_Descriptor_Device_t getPluggedInDescriptor(void);
typedef enum {
    STANDARD_ACK,
    ABORTED_ACK,
    LAST_ACK_END,
    NEXT_ACK_DATA,
} PacketAction_t;
typedef struct {
    uint id;
    uint id_crc;
    uint oneCount;
    uint current_packet;
    uint8_t buffer[MAX_PACKET_LEN];
    uint8_t bufferCRC[MAX_PACKET_LEN];
    uint8_t packets[MAX_PACKET_COUNT][MAX_PACKET_LEN];
    uint8_t packetlens[MAX_PACKET_COUNT];
    uint8_t packetresplens[MAX_PACKET_COUNT];
    uint8_t packet_actions[MAX_PACKET_COUNT];
    bool lastJ;
} state_t;
extern bool full_speed;