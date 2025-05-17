#include "MIDIData.hpp"
MidiData::MidiData() {
    
}
MidiData MidiData::instance = MidiData();
void onNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    // velocities are 7 bit
    // printf("Note ON ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    MidiData::instance.midiVelocities[note] = velocity;
}

void offNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    // printf("Note OFF ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    MidiData::instance.midiVelocities[note] = 0;
}

void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2) {
    // cc are 7 bit
    if (b1 == MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL) {
        MidiData::instance.midiSustainPedal = b2 << 1;
    }
    if (b1 == MIDI_CONTROL_COMMAND_MOD_WHEEL) {
        MidiData::instance.midiModWheel = b2 << 1;
    }
}

void onPitchBend(uint8_t channel, int pitch) {
    // pitchbend is signed 14 bit
    MidiData::instance.midiPitchWheel = pitch << 2;
}

template <class Transport>
void MidiData::registerInterface(MIDI_NAMESPACE::MidiInterface<Transport> interface) {
    interface.setHandleNoteOn(onNote);
    interface.setHandleNoteOff(offNote);
    interface.setHandleControlChange(onControlChange);
    interface.setHandlePitchBend(onPitchBend);
}