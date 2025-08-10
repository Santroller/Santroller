#include "band_hero_drum.hpp"
void BandHeroDrum::tick() {
    uint8_t data[5];
    connected = interface.readRegister(DRUM_ADDR, BH_DRUM_PTR, sizeof(data), data);
    if (connected) {
        midiInterface.parsePacket(data+2, sizeof(data)-2);
    }
};