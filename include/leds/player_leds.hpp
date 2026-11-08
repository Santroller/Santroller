#pragma once
#include <stdint.h>
#include "enums.pb.h"

// Which of a player LED mapping's LEDs show player (1 based, 0 for none) in style, bit 0 being the first
// LED listed. The patterns are what the consoles' own controllers show, as the Linux drivers for them do.
inline uint8_t player_led_pattern(PlayerLedStyle style, uint8_t player)
{
    static constexpr uint8_t ps3[] = {0x01, 0x02, 0x04, 0x08, 0x09, 0x0A, 0x0C, 0x0D, 0x0E, 0x0F};
    static constexpr uint8_t sw[] = {0x01, 0x03, 0x07, 0x0F, 0x09, 0x05, 0x0D, 0x06};
    static constexpr uint8_t ps5[] = {0x04, 0x0A, 0x15, 0x1B, 0x1F};
    if (player == 0)
    {
        return 0;
    }
    uint8_t i = player - 1;
    switch (style)
    {
    case PlayerLedSequential:
        return i < 8 ? 1 << i : 0;
    case PlayerLedPlayStation3:
        return i < sizeof(ps3) ? ps3[i] : 0;
    case PlayerLedSwitch:
        return i < sizeof(sw) ? sw[i] : 0;
    case PlayerLedPlayStation5:
        return i < sizeof(ps5) ? ps5[i] : 0;
    }
    return 0;
}
