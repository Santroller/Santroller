#pragma once
#include <stdint.h>
typedef struct {
    uint8_t midiVelocities[128];
    int16_t midiPitchWheel;
    uint8_t midiModWheel;
    uint8_t midiSustainPedal;
} san_midi_t;