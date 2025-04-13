#include "devices/midi.hpp"
#include "devices/bh_drum.hpp"
#include "devices/wt_drum.hpp"
#include "devices/wii.hpp"
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
MIDIDevice<Transport>::MIDIDevice(Transport transport) : mMidiTransport(transport), mMidi(transport) {
    mMidi.setHandleNoteOn(onNote);
    mMidi.setHandleNoteOff(offNote);
    mMidi.setHandleControlChange(onControlChange);
    mMidi.setHandlePitchBend(onPitchBend);
}

template <typename Transport>
void MIDIDevice<Transport>::tick(san_base_t* data) {
    mMidiTransport.tick(data);
    if (mMidi.read()) {
        memcpy(data->midi.midiVelocities, midiVelocities, sizeof(midiVelocities));
        data->midi.midiModWheel = midiModWheel;
        data->midi.midiPitchWheel = midiPitchWheel;
        data->midi.midiSustainPedal = midiSustainPedal;
    }
}
template <typename Transport>
void MIDIDevice<Transport>::begin() {
    mMidi.begin();
}
template class MIDIDevice<BHMidiTransport>;
template class MIDIDevice<WTMidiTransport>;
template class MIDIDevice<WiiDevice>;