#pragma once
#include <stdint.h>
typedef struct {
    uint8_t midiVelocities[128];
    int16_t midiPitchWheel;
    uint8_t midiModWheel;
    uint8_t midiSustainPedal;
    uint8_t midiPacket[6]; // GH drum kits contain a raw midi packet, so we store that here so we can process it later
} san_midi_t;