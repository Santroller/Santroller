#pragma once
// Take inputs from the midi library
// Have a seperate mode for drums and everything else, handling the triggers in a nicer way
// perhaps we do something where we handle note duration properly for drums instead of it being a hack on top of debounce
#include <MIDI.h>

#include "state/base.hpp"
#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
using namespace MIDI_NAMESPACE;

static uint8_t midiVelocities[128];
static int16_t midiPitchWheel;
static uint8_t midiModWheel;
static uint8_t midiSustainPedal;
inline void onNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    // velocities are 7 bit
    printf("Note ON ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    midiVelocities[note] = velocity;
}

inline void offNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    printf("Note OFF ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    midiVelocities[note] = 0;
}

static void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2) {
    // cc are 7 bit
    printf("ControlChange ch=%d, b1=%d, b2=%d\r\n", channel, b1, b2);
    if (b1 == MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL) {
        midiSustainPedal = b2 << 1;
    }
    if (b1 == MIDI_CONTROL_COMMAND_MOD_WHEEL) {
        midiModWheel = b2 << 1;
    }
}

static void onPitchBend(uint8_t channel, int pitch) {
    // pitchbend is signed 14 bit
    printf("PitchBend ch=%d, pitch=%d\r\n", channel, pitch);
    midiPitchWheel = pitch << 2;
}
template <typename Transport>
class MIDIDevice {
   public:
    inline MIDIDevice(Transport transport) : mMidiTransport(transport), mMidi(transport) {
        mMidi.setHandleNoteOn(onNote);
        mMidi.setHandleNoteOff(offNote);
        mMidi.setHandleControlChange(onControlChange);
        mMidi.setHandlePitchBend(onPitchBend);
    }

    void begin() {
        mMidi.begin();
    }

   private:
    Transport mMidiTransport;
    MIDI_NAMESPACE::MidiInterface<Transport> mMidi;
    void tick(san_base_t* data) {
        if (mMidi.read()) {
            memcpy(data->midi.midiVelocities, midiVelocities, sizeof(midiVelocities));
            data->midi.midiModWheel = midiModWheel;
            data->midi.midiPitchWheel = midiPitchWheel;
            data->midi.midiSustainPedal = midiSustainPedal;
        }
    }
};