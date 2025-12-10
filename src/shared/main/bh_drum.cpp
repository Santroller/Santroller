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
    uint8_t header[2];
    uint8_t data[3];
    bh_drum_found = twi_readFromPointer(BH_DRUM_TWI_PORT, DRUM_ADDR, BH_DRUM_PTR, sizeof(header), header);
    if (bh_drum_found)
    {
        // Stream out the rest of the packets byte by byte as the drums lock up otherwise.
        uint8_t count = header[1] >> 4;
        for (int i = 0; i < count; i++)
        {
            bh_drum_found = twi_readFrom(BH_DRUM_TWI_PORT, DRUM_ADDR, data, sizeof(data), true);
            uint8_t type = (data[0] & 0xf0);
            uint8_t channel = data[0] & 0x0f;
            // TODO: CC and stuff, can the midi lib parse this all for us?
            if (type == 0x90)
            {
                TRANSLATE_GH_MIDI(channel, data[1], data[2], onNote);
            }
            else if (type == 0x80)
            {
                TRANSLATE_GH_MIDI(channel, data[1], data[2], offNote);
            }
        }
    }
}
#endif