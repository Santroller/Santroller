#include <stdint.h>

#include "Arduino.h"
#include "io.h"
#include "shared_main.h"
#include "state_translation/drums.h"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
#ifdef BH_DRUM_TWI_PORT
bool bh_drum_found = false;
static long lastTick = 0;
static int missing = 0;
void tickBhDrum()
{
    if (micros() - lastTick > 20000)
    {
        lastTick = micros();
        // BH Drum format: 0x10 0x<packetsinbuffer>0 <midi packet (of len 3, if you request more bytes it will just return multiple packets)>
        uint8_t data[5];
        bh_drum_found = twi_readFromPointer(BH_DRUM_TWI_PORT, DRUM_ADDR, BH_DRUM_PTR, sizeof(data), data);
        if (bh_drum_found)
        {
            uint8_t status = data[2];
            uint8_t type = (status & 0xf0);
            uint8_t channel = status & 0x0f;
            // TODO: CC and stuff, can the midi lib parse this all for us?
            if (type == 0x90)
            {
                uint8_t note = data[3];
                uint8_t velocity = data[4];
                TRANSLATE_GH_MIDI(channel, note, velocity, onNote);
            }
            else if (type == 0x80)
            {
                uint8_t note = data[3];
                uint8_t velocity = data[4];
                TRANSLATE_GH_MIDI(channel, note, velocity, offNote);
            }
        }
    }
}
#endif