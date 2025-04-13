#pragma once
#include <MIDI.h>

#include "interfaces/i2c.hpp"
using namespace MIDI_NAMESPACE;
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class BHMidiTransport {
    friend class MIDI_NAMESPACE::MidiInterface<BHMidiTransport>;

   public:
    BHMidiTransport(I2CMasterInterface* interface)
        : mInterface(interface) {};

    void begin() {
    }

    void end() {
    }

   protected:
    static const bool thruActivated = false;
    // Read only, we don't ever send
    bool beginTransmission(MidiType) {
        return false;
    };

    void write(byte byte) {

    };

    void endTransmission() {
    };

    byte read() {
        return mBuffer[mBufferIndex];
    };

    unsigned available() {
        if (mBufferIndex > 0) {
            mBufferIndex--;
        } else {
            uint8_t data[8];
            if (mInterface->readFromPointer(DRUM_ADDR, BH_DRUM_PTR, sizeof(data), data) && data[0] == BH_DRUM_PTR) {
                memcpy(mBuffer, data + 1, sizeof(data) - 1);
                mBufferIndex = sizeof(data) - 1;
            }
        }
        return mBufferIndex;
    };

   private:
    I2CMasterInterface* mInterface;
    uint8_t mBufferIndex;
    uint8_t mBuffer[8];
};
