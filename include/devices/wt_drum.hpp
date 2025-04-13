#pragma once
#include <MIDI.h>

#include "interfaces/core.hpp"
#include "interfaces/gpio.hpp"
#include "interfaces/spi.hpp"
#include "state/base.hpp"
using namespace MIDI_NAMESPACE;
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class WTMidiTransport {
    friend class MIDI_NAMESPACE::MidiInterface<WTMidiTransport>;

   public:
    WTMidiTransport(SPIMasterInterface* interface, GPIOInterface* csPin)
        : mInterface(interface), mCsPin(csPin) {};

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
    SPIMasterInterface* mInterface;
    GPIOInterface* mCsPin;
    uint8_t mBufferIndex;
    uint8_t mBuffer[8];
    long lastTick;
    int missing;
    bool mFound;
};