#pragma once
// Take inputs from the midi library
// Have a seperate mode for drums and everything else, handling the triggers in a nicer way
// perhaps we do something where we handle note duration properly for drums instead of it being a hack on top of debounce
#include <MIDI.h>

#include "state/base.hpp"
#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
using namespace MIDI_NAMESPACE;

extern void onNote(uint8_t channel, uint8_t note, uint8_t velocity);
extern void offNote(uint8_t channel, uint8_t note, uint8_t velocity);
extern void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2);
extern void onPitchBend(uint8_t channel, int pitch);
extern uint8_t midiVelocities[128];
extern int16_t midiPitchWheel;
extern uint8_t midiModWheel;
extern uint8_t midiSustainPedal;
template <typename Transport>
class MIDIDevice {
   public:
    MIDIDevice(Transport transport) : mMidiTransport(transport), mMidi(transport) {
        mMidi.setHandleNoteOn(onNote);
        mMidi.setHandleNoteOff(offNote);
        mMidi.setHandleControlChange(onControlChange);
        mMidi.setHandlePitchBend(onPitchBend);
    }

    void begin() {
        mMidi.begin();
    }

    template <class U = Transport, decltype(U{}.tick()) = true>
    void tick(san_base_t* data) {
        mMidiTransport.tick(data);
        tickMidi(data);
    }
    void tick(san_base_t* data) {
        tickMidi(data);
    }

   private:
    Transport mMidiTransport;
    MIDI_NAMESPACE::MidiInterface<Transport> mMidi;

    void tickMidi(san_base_t* data) {
        if (mMidi.read()) {
            memcpy(data->midi.midiVelocities, midiVelocities, sizeof(midiVelocities));
            data->midi.midiModWheel = midiModWheel;
            data->midi.midiPitchWheel = midiPitchWheel;
            data->midi.midiSustainPedal = midiSustainPedal;
        }
    }
};