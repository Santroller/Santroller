#pragma once
#include <MIDI.h>

#include "devices/midi.hpp"
#include "interfaces/i2c.hpp"
#include "state/base.hpp"
using namespace MIDI_NAMESPACE;
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class BHMidiTransport {
    friend class MIDI_NAMESPACE::MidiInterface<BHMidiTransport>;

   public:
    BHMidiTransport(I2CMasterInterface* interface)
        : mInterface(interface) {};

    inline void begin() {
    }

    inline void end() {
    }

   protected:
    static const bool thruActivated = false;
    // Read only, we don't ever send
    inline bool beginTransmission(MidiType) {
        return false;
    };

    inline void write(byte byte) {

    };

    inline void endTransmission() {
    };

    inline byte read() {
        return mBuffer[mBufferIndex];
    };

    unsigned available();

   private:
    I2CMasterInterface* mInterface;
    uint8_t mBufferIndex;
    uint8_t mBuffer[8];
};
