#pragma once
#include <stdint.h>
#include "emulation/usb/usb_devices.h"

#define SWITCH_PRO_CON_FULL_REPORT_ID 0x30
#define SWITCH_PRO_CON_SIMPLE_REPORT_ID 0x3F
#define SWITCH_PRO_CON_SUBCMD_REPORT_ID 0x01

// Output report 0x01: packet counter, 8 bytes of rumble (neutral here), then the subcommand and its arguments.
// Subcommand 0x03 sets the input report mode, 0x30 being the full report with 12 bit sticks. Over Bluetooth
// the controller starts out in the simple 0x3F mode until it gets this.
static const uint8_t SWITCH_SET_FULL_REPORT_MODE[] = {SWITCH_PRO_CON_SUBCMD_REPORT_ID, 0x00,
                                                      0x00, 0x01, 0x40, 0x40, 0x00, 0x01, 0x40, 0x40,
                                                      0x03, 0x30};

// ---------------------------------------------------------------------------
// Rumble and player LEDs, shared by the USB and Bluetooth hosts
// ---------------------------------------------------------------------------
#define SWITCH_RUMBLE_REPORT_ID 0x10
#define SWITCH_SUBCMD_SET_PLAYER_LIGHTS 0x30
#define SWITCH_RUMBLE_REPORT_LEN 10
#define SWITCH_PLAYER_LED_REPORT_LEN 12
#define SWITCH_OUTPUT_MAX_LEN SWITCH_PLAYER_LED_REPORT_LEN
// Same timings as SDL: rumble writes at most every 30ms, and a running rumble has to be refreshed
// every 50ms or the motors stop
#define SWITCH_OUTPUT_MIN_INTERVAL_MS 30
#define SWITCH_RUMBLE_REFRESH_MS 50

enum class SwitchOutputKind : uint8_t
{
    None,
    Rumble,
    PlayerLed
};

// Output bookkeeping, the transport specific hosts only do the actual send. build() fills in the next
// report (report id first) without changing anything, then sent() or failed() once the outcome is known.
struct SwitchOutput
{
    uint8_t strong = 0;
    uint8_t weak = 0;
    uint8_t player = 0;
    bool rumble_dirty = true;
    bool player_dirty = true;
    uint8_t counter = 0;
    uint32_t last_sent_ms = 0;

    void set_rumble(uint8_t left, uint8_t right)
    {
        if (left != strong || right != weak)
            rumble_dirty = true;
        strong = left;
        weak = right;
    }

    void set_player(uint8_t p)
    {
        if (p != player)
            player_dirty = true;
        player = p;
    }

    // Defined in the USB Switch host, which the Bluetooth host shares
    uint8_t build(uint32_t now, uint8_t *out, SwitchOutputKind &kind) const;

    void sent(SwitchOutputKind kind, uint32_t now)
    {
        if (kind == SwitchOutputKind::None)
            return;
        counter++;
        last_sent_ms = now;
        rumble_dirty = false;
        if (kind == SwitchOutputKind::PlayerLed)
            player_dirty = false;
    }

    void failed(SwitchOutputKind kind)
    {
        if (kind == SwitchOutputKind::None)
            return;
        rumble_dirty = true;
        if (kind == SwitchOutputKind::PlayerLed)
            player_dirty = true;
    }
};
typedef struct
{
    int16_t accX, accY, accZ;
    int16_t gyroX, gyroY, gyroZ;
} __attribute__((packed)) ImuData;


typedef struct
{
    uint8_t connectionInfo : 4;
    uint8_t batteryLevel : 4;

    // byte 00
    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t buttonRightSR : 1;
    uint8_t buttonRightSL : 1;
    uint8_t rightShoulder : 1;
    uint8_t rightTrigger : 1;

    uint8_t back : 1;            // minus
    uint8_t start : 1;           // plus
    uint8_t rightThumbClick : 1; // r3
    uint8_t leftThumbClick : 1;  // l3
    uint8_t guide : 1;           // home
    uint8_t capture : 1;
    uint8_t dummy2 : 1;
    uint8_t chargingGrip : 1;

    uint8_t dpadDown : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;

    uint8_t buttonLeftSR : 1;
    uint8_t buttonLeftSL : 1;
    uint8_t leftShoulder : 1;
    uint8_t leftTrigger : 1;

    uint16_t leftStickX : 12;
    uint16_t leftStickY : 12;
    uint16_t rightStickX : 12;
    uint16_t rightStickY : 12;
} __attribute__((packed)) SwitchInputReport;

typedef struct
{
    uint8_t reportID;
    uint8_t timestamp;
    SwitchInputReport inputs;
    uint8_t rumbleReport;
    uint8_t imuData[36];
    uint8_t padding[15];
} __attribute__((packed)) SwitchProGamepad_Data_t;

typedef struct
{
    uint8_t reportId; // 0x0A
    uint8_t packetId; // counts up for each packet
    uint8_t status;   // 0x20
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t r2 : 1;
    uint8_t rightShoulder : 1;
    uint8_t start : 1;
    uint8_t rightThumbClick : 1;

    uint8_t dpadDown : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadUp : 1;
    uint8_t l2 : 1;
    uint8_t leftShoulder : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;

    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t gl : 1;
    uint8_t gr : 1;
    uint8_t c : 1;
    uint8_t : 3;

    uint16_t leftStickX : 12;
    uint16_t leftStickY : 12;
    uint16_t rightStickX : 12;
    uint16_t rightStickY : 12;
    uint8_t vibrationCode;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t imuLength;
    uint8_t motionBuffer[10];
} __attribute__((packed)) Switch2ProGamepadGC_Data_t;

typedef struct
{
    uint8_t reportId; // 0x09
    uint8_t packetId; // counts up for each packet
    uint8_t status;   // 0x20
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t y : 1;
    uint8_t x : 1;
    uint8_t rightShoulder : 1;
    uint8_t r2 : 1;
    uint8_t start : 1;
    uint8_t rightThumbClick : 1;

    uint8_t dpadDown : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadUp : 1;
    uint8_t leftShoulder : 1;
    uint8_t l2 : 1;
    uint8_t back : 1;
    uint8_t leftThumbClick : 1;

    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t gl : 1;
    uint8_t gr : 1;
    uint8_t c : 1;
    uint8_t : 3;

    uint16_t leftStickX : 12;
    uint16_t leftStickY : 12;
    uint16_t rightStickX : 12;
    uint16_t rightStickY : 12;
    uint8_t vibrationCode;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    uint8_t imuLength;
    uint8_t motionBuffer[10];
} __attribute__((packed)) Switch2ProGamepad_Data_t;

typedef struct
{
    // Button bits
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;

    uint8_t leftShoulder : 1;  // l1
    uint8_t rightShoulder : 1; // r1
    uint8_t l2 : 1;            // l2
    uint8_t r2 : 1;            // r2

    uint8_t back : 1; // select
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;  // l3
    uint8_t rightThumbClick : 1; // r3

    uint8_t guide : 1;   // ps
    uint8_t capture : 1; // switch capture button
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    uint8_t dpadUp : 1;
    uint8_t dpadDown : 1;
    uint8_t dpadLeft : 1;
    uint8_t dpadRight : 1;
    uint8_t : 4;

    // Stick axes
    // Neutral state is 0x80
    // X axis is left at 0x00, right at 0xFF
    // Y axis is top at 0x00, bottom at 0xFF
    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t rightStickX;
    uint8_t rightStickY;

    uint8_t unused2[12];
    uint16_t unused3[4];

} __attribute__((packed)) SwitchGamepad_Data_t;

// Festivals default mappings suck, so its easiest to set up custom mappings for that here
typedef struct
{
    uint8_t y : 1;             // y, triangle, yellow
    uint8_t rightShoulder : 1; // b, circle, red
    uint8_t leftShoulder : 1;  // a, cross, green
    uint8_t x : 1;             // x, square, blue

    uint8_t dpadUp : 1;   // leftShoulder, orange, l1
    uint8_t dpadDown : 1; // rightShoulder, spPedal, r1
    uint8_t back : 1;     // back, select
    uint8_t start : 1;    // start

    uint8_t guide : 1; // ps
    uint8_t capture :1;
    uint8_t : 1;
    uint8_t whammy : 1;
    uint8_t tilt : 1;
    uint8_t : 2;

    // To make things easier, we use bitfields here, and then we map to a proper hat later
    union
    {
        struct
        {
            uint8_t b : 1;         // dpadUp
            uint8_t dpadRight : 1; // dpadDown
            uint8_t a : 1;         // dpadLeft
            uint8_t dpadLeft : 1;  // dpadRight
            uint8_t : 4;
        };
        uint8_t dpad;
    };

    uint8_t leftStickX;
    uint8_t leftStickY;
    uint8_t slider;
} __attribute__((packed)) SwitchFestivalProGuitarLayer_Data_t;

typedef struct
{
    uint8_t y : 1;
    uint8_t b : 1;
    uint8_t a : 1;
    uint8_t x : 1;
    uint8_t leftShoulder : 1;
    uint8_t rightShoulder : 1;
    uint8_t l2 : 1; // l2 rimLeft
    uint8_t r2 : 1; // r2 rimRight

    uint8_t back : 1;
    uint8_t start : 1;
    uint8_t leftThumbClick : 1;  // l3 centerLeft
    uint8_t rightThumbClick : 1; // r3 centerRight
    uint8_t guide : 1;
    uint8_t capture : 1;
    uint8_t : 2;

    uint8_t dpad;
    uint8_t sticks[4]; // always 0x80
    uint8_t : 8;

} __attribute__((packed)) SwitchTaiko_Data_t;