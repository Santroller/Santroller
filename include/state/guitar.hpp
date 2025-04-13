#pragma once
#include <stdint.h>
#include "gamepad.hpp"
typedef struct {
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;

    // Solo also gets reused for slider
    uint8_t soloGreen : 1;
    uint8_t soloRed : 1;
    uint8_t soloYellow : 1;
    uint8_t soloBlue : 1;
    uint8_t soloOrange : 1;
    uint8_t strumUp : 1;
    uint8_t strumDown : 1;
    uint8_t pedal : 1;
    uint8_t : 3;

    uint8_t whammy;
    uint8_t pickup;
    uint8_t tilt;
} san_gh_rb_guitar_t;