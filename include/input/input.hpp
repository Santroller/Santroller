#pragma once
#include <stdint.h>
#include "input_enums.pb.h"
class MidiNoteInput;
class ShortcutInput;

enum InputHardwareType : uint8_t {
    InputHw_None = 0,
    InputHw_GPIO = 1,
    InputHw_WiiButton = 2,
    InputHw_PS2Button = 3,
    InputHw_Matrix = 4,
    InputHw_MPR121 = 5,
    InputHw_USBButton = 6,
    InputHw_KeyboardKey = 7,
    InputHw_Crkd = 8,
    InputHw_VTechExpander = 9,
    InputHw_Gh5Neck = 10,
    InputHw_ProtarNeck = 11,
    InputHw_MidiNote = 12,
    InputHw_Multiplexer = 13,
};

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
    virtual MidiNoteInput* as_midi_note() { return nullptr; }
    virtual ShortcutInput* as_shortcut() { return nullptr; }
    virtual uint64_t hardware_id() const { return 0; }
};


class DrumState {
public:
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

    void reset()
    {
        blue_cymbal = 0;
        blue_pad = 0;
        yellow_cymbal = 0;
        yellow_pad = 0;
        green_cymbal = 0;
        green_pad = 0;
        red_pad = 0;
    }

    bool has_hits() const
    {
        return red_pad || yellow_cymbal || yellow_pad || blue_cymbal || blue_pad || green_cymbal || green_pad;
    }
};

class KeyboardState {
    public:
    uint32_t pressed_keys = 0;
    uint8_t last_seen_keys[10];
};