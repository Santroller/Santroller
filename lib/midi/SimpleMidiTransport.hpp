#pragma once
#include "MIDI.h"
#include "MIDIData.hpp"
BEGIN_MIDI_NAMESPACE
class SimpleMidiTransport {
    friend class MidiInterface<SimpleMidiTransport>;
    static const bool thruActivated = false;
    // Read only, we don't ever send
    inline bool beginTransmission(MidiType) {
        return false;
    };
    inline void begin() {
    }

    inline void end() {
    }

    inline void write(byte byte) {

    };

    inline void endTransmission() {
    };

    inline byte read() {
        return nextByte;
    };

    inline unsigned available() {
        return 1;
    }

   public:
    uint8_t nextByte;
};

class SimpleMidiInterface {
   public:
    SimpleMidiInterface() : midiInterface(transport) {
        // MidiData::instance.registerInterface(midiInterface);
    }
    void parsePacket(uint8_t* data, uint8_t len) {
        midiInterface.begin();
        for (int i = 0; i < len; i++) {
            transport.nextByte = data[i];
            midiInterface.read();
        }
    }

   private:
    SimpleMidiTransport transport;
    MidiInterface<SimpleMidiTransport> midiInterface;
};

END_MIDI_NAMESPACE