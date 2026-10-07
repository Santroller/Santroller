#pragma once
#include <stdint.h>

// Pickup switch values for each of the 5 notches. RB4 guitars (PS4 / PS5) report the notch
// index (0 - 4) directly instead.
static const uint8_t rb_pickup_universal[] = {0x19, 0x4c, 0x96, 0xb2, 0xe5};
static const uint8_t rb_pickup_xbox_one[] = {0x00, 0x10, 0x20, 0x30, 0x40};

// A notch index as a full range axis value, matching the pickup values from other guitars
inline uint16_t rb_pickup_notch_value(uint8_t notch)
{
    return rb_pickup_universal[notch > 4 ? 4 : notch] << 8;
}
