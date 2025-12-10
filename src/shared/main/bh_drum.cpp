#include <stdint.h>

#include "Arduino.h"
#include "io.h"
#include "shared_main.h"
#include "state_translation/drums.h"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
#ifdef BH_DRUM_TWI_PORT
bool bh_drum_found = false;
void tickBhDrum()
{
    // Drum packet starts with some counter, and then the number of packets in the buffer
    uint8_t data[2];
    bh_drum_found = twi_readFromPointer(BH_DRUM_TWI_PORT, DRUM_ADDR, BH_DRUM_PTR, sizeof(data), data);
    if (bh_drum_found)
    {
        // Stream out the rest of the packets byte by byte as the drums lock up otherwise.
        uint8_t count = data[1] >> 4;
        for (int i = 0; i < count; i++)
        {
            uint8_t status;
            uint8_t note;
            uint8_t velocity;
            bh_drum_found = twi_readFrom(BH_DRUM_TWI_PORT, DRUM_ADDR, &status, 1, 1);
            bh_drum_found = twi_readFrom(BH_DRUM_TWI_PORT, DRUM_ADDR, &note, 1, 1);
            bh_drum_found = twi_readFrom(BH_DRUM_TWI_PORT, DRUM_ADDR, &velocity, 1, 1);
            uint8_t type = (status & 0xf0);
            uint8_t channel = status & 0x0f;
            // TODO: CC and stuff, can the midi lib parse this all for us?
            if (type == 0x90)
            {
                TRANSLATE_GH_MIDI(channel, note, velocity, onNote);
            }
            else if (type == 0x80)
            {
                TRANSLATE_GH_MIDI(channel, note, velocity, offNote);
            }
        }
    }
}
#endif