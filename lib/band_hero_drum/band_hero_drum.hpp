// Support for band hero drums as midi devices
#pragma once
#include "i2c.hpp"
#include "devices/midi.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
#define MAX_PACKETS_IN_BUFFER 0x0F
#define PACKET_SIZE 3
class BandHeroDrum {
   public:
    BandHeroDrum(MidiDevice* midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock), m_device(midiDevice) {};
    void tick();
    inline bool is_connected() {
        return connected;
    }

   private:
    I2CMasterInterface interface;
    bool connected;
    long m_lastTick = 0;
    uint8_t m_lastCount = 0;
    MidiDevice *m_device;
};
