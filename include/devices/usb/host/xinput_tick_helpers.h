#pragma once

#include "protocols/xinput.hpp"
#include "config.pb.h"

// Shared tick_digital and tick_analog implementations for XInput devices, in
// src/devices/usb/host/xinput_tick_helpers.cpp
// These functions take the buffer and subtype as parameters

// RB velocities are inverted so RB1 kits, which always send 0, read as the hardest hit: the
// closer to 0 the harder the hit. Red and blue are positive (0 to 32767), yellow and green are
// negative while hit (-1 to -32768). Only meaningful while the matching colour is hit.
uint16_t xinput_rb_drum_velocity(int16_t velocity);

struct XInputRockBandDrumHits
{
    uint16_t red_pad;
    uint16_t yellow_pad;
    uint16_t blue_pad;
    uint16_t green_pad;
    uint16_t yellow_cymbal;
    uint16_t blue_cymbal;
    uint16_t green_cymbal;
};

// RB1 kits have no velocity or cymbals and report every hit as a plain face button. RB2+ kits
// flag pads and cymbals and use the d-pad to tell which colour a cymbal hit was. Hosts tell
// them apart with XINPUT_CAPS_BUTTON_RIGHT_THUMB in the kit's capabilities.
XInputRockBandDrumHits xinput_decode_rb_drums(const XInputRockBandDrums_Data_t *data, bool rb2);

uint16_t xinput_rb_drum_axis(const uint8_t *ep_in_buf, RockBandDrumsAxisType axis, bool rb2);

// Guitar Hero kits report standard 7 bit MIDI velocities
uint16_t xinput_gh_drum_axis(const uint8_t *ep_in_buf, GuitarHeroDrumsAxisType axis);

bool xinput_tick_digital_impl(const uint8_t* ep_in_buf, SubType subtype, proto_Output &type, bool is_wt = false, bool rb2 = false);

uint16_t xinput_turntable_velocity(int16_t velocity);

uint16_t xinput_tick_analog_impl(const uint8_t* ep_in_buf, SubType subtype, proto_Output &type, bool rb2 = false);

uint16_t xinput_tick_button_pressure_impl(const uint8_t *ep_in_buf, SubType subtype,
                                          proto_Output &type, bool is_wt = false, bool rb2 = false);
