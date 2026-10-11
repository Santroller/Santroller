#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input.pb.h"

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

// Santroller 2 sends the gamepad stick Y axes flipped to the HID convention, down at the max
// (GamepadAxisMapping::update_hid), where the rest of its XInput style report has up at the
// max, unless it reports CapabilityXInputYAxis. Flips a value the xinput tick helpers read from
// the report back to XInput's convention.
static inline uint16_t santroller_v2_fix_analog(const proto_Output &type, uint16_t value, uint8_t capabilities)
{
    if (!(capabilities & CapabilityXInputYAxis) && type.which_mapping == proto_Output_gamepadAxis_tag &&
        (type.mapping.gamepadAxis == Gamepad_LeftStickY || type.mapping.gamepadAxis == Gamepad_RightStickY))
        return ~value;
    return value;
}
