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
    InputHw_BTButton = 14,
    InputHw_BTAxis = 15,
    InputHw_Encoder = 16,
    InputHw_SwitchNetwork = 17,
    InputHw_InfiniumFader = 18,
    InputHw_Peripheral = 19,
};

class Input
{
public:
    Input(){}
    virtual ~Input(){}
    virtual bool tick_digital() = 0;
    virtual uint16_t tick_analog() = 0;
    virtual void setup() = 0;
    virtual bool valid() const { return true; }
    virtual bool has_independent_analog_value() const { return false; }
    virtual bool consumes_events() const { return false; }
    virtual bool consume_event(uint16_t &value) { (void)value; return false; }
    virtual bool peek_event(uint16_t &value) { (void)value; return false; }
    virtual MidiNoteInput* as_midi_note() { return nullptr; }
    virtual ShortcutInput* as_shortcut() { return nullptr; }
    virtual bool tick_pro_key_range(uint32_t &active_keys, uint8_t *velocities, uint8_t key_count)
    {
        (void)active_keys;
        (void)velocities;
        (void)key_count;
        return false;
    }
    virtual uint64_t hardware_id() const { return 0; }
    virtual void link_device(bool claim_devices) {}
};


class DrumState {
public:
    RockBandDrumsAxisType last_drum = (RockBandDrumsAxisType)0;
    RockBandDrumsAxisType buffered_cymbal = (RockBandDrumsAxisType)0;
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
    uint8_t pressed_keys[32] = {0};
    uint8_t last_seen_keys[10] = {0};

    void set_key(uint8_t keycode) {
        pressed_keys[keycode / 8] |= (1 << (keycode % 8));
    }

    void clear_key(uint8_t keycode) {
        pressed_keys[keycode / 8] &= ~(1 << (keycode % 8));
    }

    bool is_key_pressed(uint8_t keycode) const {
        return (pressed_keys[keycode / 8] & (1 << (keycode % 8))) != 0;
    }

    void clear_all() {
        memset(pressed_keys, 0, sizeof(pressed_keys));
    }
};