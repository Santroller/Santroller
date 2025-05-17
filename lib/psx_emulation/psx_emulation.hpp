#pragma once
#include <stdint.h>

#include "spi.hpp"


typedef enum {
    PSX_UNKNOWN_CONTROLLER = 0,
    PSX_DIGITAL,
    PSX_DUALSHOCK_1_CONTROLLER,
    PSX_DUALSHOCK_2_CONTROLLER,
    PSX_GUITAR_HERO_CONTROLLER,
    PSX_NEGCON,
    PSX_JOGCON,
    PSX_GUNCON,
    PSX_FLIGHTSTICK,
    PSX_MOUSE,
    PSX_NO_DEVICE
} PsxControllerType_t;
class PSXEmulation {
};