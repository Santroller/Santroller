#pragma once
#include <MIDI.h>

#include "interfaces/i2c.hpp"
#include "parsers/wii.hpp"
#include "state/base.hpp"
using namespace MIDI_NAMESPACE;
#define WII_ADDR 0x52
#define WII_READ_ID 0xFA
#define WII_ENCRYPTION_STATE_ID 0xF0
#define WII_ENCRYPTION_ENABLE_ID 0xAA
#define WII_ENCRYPTION_FINISH_ID 0x55
#define WII_ENCRYPTION_KEY_ID 0x40
#define WII_ENCRYPTION_KEY_ID_2 0x46
#define WII_ENCRYPTION_KEY_ID_3 0x4C
#define WII_ID_LEN 6
#define WII_DJ_EUPHORIA 0xFB
#define WII_SET_RES_MODE 0xFE
#define WII_LOWRES_MODE 0x03
#define WII_HIGHRES_MODE 0x03
#define FIRST_PARTY_SBOX 0x97
#define THIRD_PARTY_SBOX 0x4D

class WiiDevice {
    friend class MIDI_NAMESPACE::MidiInterface<WiiDevice>;

   public:
    inline WiiDevice(I2CMasterInterface* interface) : mInterface(interface), mFound(false) {
    }
    void tick(san_base_t* data);
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
    bool verifyData(const uint8_t* dataIn, uint8_t dataSize);
    WiiExtType_t readExtID();
    void initWiiExt();
    void setEuphoriaLed(bool state);
    I2CMasterInterface* mInterface;
    WiiParser parser;
    bool mFound;
    bool nextEuphoriaLedState = false;
    bool ledUpdated = false;
    bool hadDrum = false;
    uint8_t packetIssueCount;
    uint8_t mBufferIndex;
    uint8_t mBuffer[8];
    long lastTick;
    uint8_t wiiBytes;
    uint8_t wiiPointer = 0;
    uint8_t s_box = 0;
};