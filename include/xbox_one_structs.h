#include <stdint.h>
#define GIP_POWER_MODE_DEVICE_CONFIG 0x05
#define GIP_ARRIVAL 0x03
#define GIP_CMD_RUMBLE   0x09
#define GIP_INPUT_REPORT 0x20
#define GHL_HID_REPORT 0x21
typedef struct
{
    uint8_t command;
    uint8_t client : 4;
    bool needsAck : 1;
    bool internal : 1;
    bool chunkStart : 1;
    bool chunked : 1;
    uint8_t sequence;
    uint8_t length;
} __attribute__((packed)) GipHeader_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t battery;
    // uint8_t batteryLevel : 2;
    // uint8_t batteryType : 2;
    // uint8_t unk1 : 4;
    uint8_t unk2[3];
} __attribute__((packed)) GipArrival_t;

typedef struct
{
    GipHeader_t Header;
    USB_PS3Report_Data_t report;
} __attribute__((packed)) GhlHidReport_t;
typedef struct
{
    GipHeader_t Header;
    uint8_t report[14];
} __attribute__((packed)) StandardControllerReport_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t report[10];
} __attribute__((packed)) RbGuitarReport_t;
typedef struct
{
    GipHeader_t Header;
    uint8_t report[6];
} __attribute__((packed)) RbDrumReport_t;

typedef struct
{
    GipHeader_t Header;
    uint8_t subcommand;
} __attribute__((packed)) GipPowerMode_t;

typedef struct {
    GipHeader_t Header;
	uint8_t unknown;
	uint8_t motors;
	uint8_t left_trigger;
	uint8_t right_trigger;
	uint8_t left;
	uint8_t right;
	uint8_t duration;
	uint8_t delay;
	uint8_t repeat;
} __attribute__((packed)) GipRumble_t;

#define GIP_HEADER(packet, cmd, isInternal, seq) \
    packet->Header.command = cmd; \
    packet->Header.internal = isInternal; \
    packet->Header.sequence = seq; \
    packet->Header.client = 0; \
    packet->Header.needsAck = 0; \
    packet->Header.chunkStart = 0; \
    packet->Header.chunked = 0; \
    packet->Header.length = sizeof(*packet) - sizeof(GipHeader_t);
