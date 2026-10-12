#pragma once

#include <array>
#include <stdint.h>
#include "enums.pb.h"

inline std::array<uint8_t, 5> psx_poll_command(PS2ControllerType type, bool rumble_enabled, uint8_t left, uint8_t right)
{
    const bool rumble = rumble_enabled &&
        (type == PS2ControllerTypeDualshock || type == PS2ControllerTypeDualshock2);
    return {0x01, 0x42, 0x00, uint8_t(rumble && right ? 0x01 : 0x00), uint8_t(rumble ? left : 0x00)};
}
