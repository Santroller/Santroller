#pragma once
#include <MIDI.h>
#include <stdint.h>

#include <SimpleMidiTransport.hpp>

#include "spi.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class WorldTourDrum {
   public:
    WorldTourDrum(SPIMasterInterface* interface, uint8_t csPin)
        : mInterface(interface), mCsPin(csPin) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }

   private:
    SPIMasterInterface* mInterface;
    uint8_t mCsPin;
    int missing;
    bool connected;
};