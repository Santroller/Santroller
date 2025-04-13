#pragma once
// Take inputs from the midi library
// Have a seperate mode for drums and everything else, handling the triggers in a nicer way
// perhaps we do something where we handle note duration properly for drums instead of it being a hack on top of debounce
#include <MIDI.h>

#include "state/base.hpp"
#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
using namespace MIDI_NAMESPACE;

template <typename Transport>
class MIDIDevice {
   public:
    MIDIDevice(Transport transport);

    void begin();

   private:
    Transport mMidiTransport;
    MIDI_NAMESPACE::MidiInterface<Transport> mMidi;
    void tick(san_base_t* data);
};