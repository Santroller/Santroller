#include "devices/midi.hpp"

#include "devices/bh_drum.hpp"
#include "devices/wii.hpp"
#include "devices/wt_drum.hpp"
uint8_t midiVelocities[128];
int16_t midiPitchWheel;
uint8_t midiModWheel;
uint8_t midiSustainPedal;
void onNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    // velocities are 7 bit
    printf("Note ON ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    midiVelocities[note] = velocity;
}

void offNote(uint8_t channel, uint8_t note, uint8_t velocity) {
    printf("Note OFF ch=%d, note=%d, vel=%d\r\n", channel, note, velocity);
    midiVelocities[note] = 0;
}

void onControlChange(uint8_t channel, uint8_t b1, uint8_t b2) {
    // cc are 7 bit
    printf("ControlChange ch=%d, b1=%d, b2=%d\r\n", channel, b1, b2);
    if (b1 == MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL) {
        midiSustainPedal = b2 << 1;
    }
    if (b1 == MIDI_CONTROL_COMMAND_MOD_WHEEL) {
        midiModWheel = b2 << 1;
    }
}

void onPitchBend(uint8_t channel, int pitch) {
    // pitchbend is signed 14 bit
    printf("PitchBend ch=%d, pitch=%d\r\n", channel, pitch);
    midiPitchWheel = pitch << 2;
}