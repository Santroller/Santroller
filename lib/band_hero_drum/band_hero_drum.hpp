// Support for band hero drums as midi devices
#pragma once
#include <MIDI.h>

#include <SimpleMidiTransport.hpp>

#include "i2c.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class BandHeroDrum {
   public:
    BandHeroDrum(I2CMasterInterface* interface)
        : interface(interface) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }

   private:
    MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    I2CMasterInterface* interface;
    bool connected;
};
