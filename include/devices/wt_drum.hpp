#pragma once
#include <MIDI.h>

#include "interfaces/core.hpp"
#include "interfaces/spi.hpp"
using namespace MIDI_NAMESPACE;
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
class WTMidiTransport {
    friend class MIDI_NAMESPACE::MidiInterface<WTMidiTransport>;

   public:
    WTMidiTransport(SPIMasterInterface* interface)
        : mInterface(interface) {};

    void begin() {
    }

    void end() {
    }

   protected:
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
            if (Core::micros() - lastTick > 500) {
                lastTick = Core::micros();
                clearCS();
                Core::delayMicroseconds(50);
                // 1: Send 0xAA, resp 0xAA
                uint8_t resp = mInterface->transfer(0xAA);
                if (resp != 0xAA) {
                    missing++;
                    if (missing > 10) {
                        mFound = false;
                        missing = 0;
                    }
                    setCS();
                    return;
                }
                mFound = true;
                // 2: Send 0x55, response: packet count in buffer
                resp = mInterface->transfer(0x55);
                delayMicroseconds(50);
                if (!resp) {
                    // no packets in buffer
                    setCS();
                    return;
                }
                // 3: read the rest of the packet
                // TODO: figure out if we can handle packets longer than 3?
                for (int i = 0; i < 3; i++) {
                    resp = mInterface->transfer(0x00);
                    mBuffer[i] = resp;
                    delayMicroseconds(50);
                }
                setCS();
                mBufferIndex = 3;
            }
        }
        return mBufferIndex;
    };

   private:
    virtual void clearCS() = 0;
    virtual void setCS() = 0;
    SPIMasterInterface* mInterface;
    uint8_t mBufferIndex;
    uint8_t mBuffer[8];
    long lastTick;
    int missing;
    bool mFound;
};