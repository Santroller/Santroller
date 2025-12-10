// Support for band hero drums as midi devices
#pragma once
#include <MIDI.h>

#include <SimpleMidiTransport.hpp>

#include "i2c.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class BandHeroDrum {
   public:
    BandHeroDrum(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }

   private:
    MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    I2CMasterInterface interface;
    bool connected;
    long m_lastTick = 0;
    uint8_t m_lastCount = 0;
};
