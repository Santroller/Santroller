#include <stdint.h>

#include "Arduino.h"
#include "io.h"
#include "shared_main.h"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
#ifdef BH_DRUM_TWI_PORT
static long lastTick = 0;
static int missing = 0;
bool wt_drum_found = false;
void tickBhDrum() {
    uint8_t data[8];
    if (twi_readFromPointer(GH5_TWI_PORT, DRUM_ADDR, BH_DRUM_PTR, sizeof(data), data) && data[0] == BH_DRUM_PTR) {
        uint8_t status = data[1];
        uint8_t type = (status & 0xf0);
        uint8_t channel = status & 0x0f;
        // TODO: CC and stuff, can the midi lib parse this all for us?
        if (type == 0x90) {
            uint8_t note = data[2];
            uint8_t velocity = data[3];
            onNote(channel, note, velocity);
        } else if (type == 0x80) {
            uint8_t note = data[2];
            uint8_t velocity = data[3];
            offNote(channel, note, velocity);
        }
    }
}
#endif