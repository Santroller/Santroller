#pragma once
#include <stdint.h>
#include "input_enums.pb.h"
class Input
{
public:
    Input(){}
    virtual ~Input(){}
    virtual bool tick_digital() = 0;
    virtual uint16_t tick_analog() = 0;
    virtual void setup() = 0;
};


class DrumState {
public:
    bool cymbal_glitch_fix = false;
    RockBandDrumsAxisType last_drum = RockBandDrums_RedPad;
    uint32_t red_pad;
    uint32_t yellow_cymbal;
    uint32_t yellow_pad;
    uint32_t blue_cymbal;
    uint32_t blue_pad;
    uint32_t green_cymbal;
    uint32_t green_pad;
    uint64_t last_global_poll;
};

class KeyboardState {
    public:
    uint32_t pressed_keys = 0;
    uint8_t last_seen_keys[10];
};