#pragma once
#include "devices/midi.hpp"
#include <stdint.h>
#include "spi.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class WorldTourDrum {
   public:
    WorldTourDrum(MidiDevice* midiDevice, int8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, uint8_t csPin);
    void tick();
    inline bool is_connected() {
        return connected;
    }

   private:
    SPIMasterInterface mInterface;
    uint8_t mCsPin;
    int missing;
    bool connected;
    MidiDevice *m_device;
    uint32_t last = 0;
};