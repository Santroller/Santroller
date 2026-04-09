#pragma once
#include "devices/midi.hpp"
#include <stdint.h>
#include "spi.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class WorldTourDrum {
   public:
    WorldTourDrum(MidiDevice* midiDevice, SPIMasterInterface* interface, uint8_t csPin)
        : mInterface(interface), mCsPin(csPin), m_device(midiDevice) {};
    void tick();
    inline bool is_connected() {
        return connected;
    }

   private:
    SPIMasterInterface* mInterface;
    uint8_t mCsPin;
    int missing;
    bool connected;
    MidiDevice *m_device;
};