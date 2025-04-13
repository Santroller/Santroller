#pragma once
#include <stdint.h>
#include "gamepad.hpp"
typedef struct {
    uint8_t green : 1;
    uint8_t red : 1;
    uint8_t yellow : 1;
    uint8_t blue : 1;
    uint8_t orange : 1;
    uint8_t pedal : 1;
    uint8_t : 2;
    uint8_t tilt;
    uint8_t lowEFret;
    uint8_t aFret;
    uint8_t dFret;
    uint8_t gFret;
    uint8_t bFret;
    uint8_t highEFret;
    uint8_t lowEFretVelocity;
    uint8_t aFretVelocity;
    uint8_t dFretVelocity;
    uint8_t gFretVelocity;
    uint8_t bFretVelocity;
    uint8_t highEFretVelocity;
} san_pro_guitar_t;