#pragma once
#include <stdint.h>

#include "MIDI.h"
#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
class MidiData {
   public:
    uint8_t midiVelocities[128];
    int16_t midiPitchWheel;
    uint8_t midiModWheel;
    uint8_t midiSustainPedal;
    static MidiData instance;
    template <class Transport>
    void registerInterface(MIDI_NAMESPACE::MidiInterface<Transport> interface);

   private:
    MidiData();
};