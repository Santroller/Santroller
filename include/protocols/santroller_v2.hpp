#pragma once
#include <stdint.h>
#include "enums.pb.h"

#define SANTROLLER_VID 0x1209
#define SANTROLLER_PID 0x2882

// Santroller 2 HID gamepad reports (report id 1) mirror the XInput layout, except the low
// nibble of byte 2 is a hat. Turn it into XInput's dpad bits (a dancepad already reports
// its dpad as a bitmask) so the shared xinput tick helpers can read the report directly.
static inline void santroller_v2_normalize_report(uint8_t *report, uint16_t len, SubType subtype)
{
    if (subtype == Dancepad || len < 3)
        return;
    uint8_t dpad_bits = 0;
    switch (report[2] & 0x0F)
    {
    case 0: dpad_bits = 0x01; break; // Up
    case 1: dpad_bits = 0x09; break; // Up + Right
    case 2: dpad_bits = 0x08; break; // Right
    case 3: dpad_bits = 0x0A; break; // Down + Right
    case 4: dpad_bits = 0x02; break; // Down
    case 5: dpad_bits = 0x06; break; // Down + Left
    case 6: dpad_bits = 0x04; break; // Left
    case 7: dpad_bits = 0x05; break; // Up + Left
    default: dpad_bits = 0; break;
    }
    report[2] = (report[2] & 0xF0) | dpad_bits;
}
