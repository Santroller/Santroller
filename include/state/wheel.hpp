#pragma once
#include <stdint.h>
#include "gamepad.hpp"
typedef struct {
    uint8_t accelerate;
    uint8_t brake;
    uint8_t wheel;
} san_wheel_t;