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
    virtual bool consumes_events() const { return false; }
    virtual bool consume_event(uint16_t &value) { (void)value; return false; }
};


class DrumState {
public:
    bool cymbal_glitch_fix = false;
    RockBandDrumsAxisType last_drum = RockBandDrums_RedPad;
    RockBandDrumsAxisType buffered_cymbal = RockBandDrums_RedPad;
    uint32_t red_pad = 0;
    uint32_t yellow_cymbal = 0;
    uint32_t yellow_pad = 0;
    uint32_t blue_cymbal = 0;
    uint32_t blue_pad = 0;
    uint32_t green_cymbal = 0;
    uint32_t green_pad = 0;
    uint32_t buffered_cymbal_value = 0;
    uint64_t last_global_poll = 0;
};

class KeyboardState {
    public:
    uint32_t pressed_keys = 0;
    uint8_t last_seen_keys[10];
};