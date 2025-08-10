#pragma once
#include <SimpleMidiTransport.hpp>

#include "i2c.hpp"
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
typedef enum {
    WII_NUNCHUK = 0x0000,
    WII_CLASSIC_CONTROLLER = 0x0001,
    WII_CLASSIC_CONTROLLER_PRO = 0x0101,
    WII_THQ_UDRAW_TABLET = 0xFF12,
    WII_UBISOFT_DRAWSOME_TABLET = 0xFF13,
    WII_GUITAR_HERO_GUITAR_CONTROLLER = 0x0003,
    WII_GUITAR_HERO_DRUM_CONTROLLER = 0x0103,
    WII_DJ_HERO_TURNTABLE = 0x0303,
    WII_TAIKO_NO_TATSUJIN_CONTROLLER = 0x0011,
    WII_MOTION_PLUS = 0x0005,
    WII_NO_EXTENSION = 0x180b,
    WII_NOT_INITIALISED = 0xFFFF
} WiiExtType_t;
class WiiExtension {

   public:
    inline WiiExtension(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock) : mInterface(block, sda, scl, clock), mFound(false) {
    }
    void tick();

   private:
    bool verifyData(const uint8_t* dataIn, uint8_t dataSize);
    WiiExtType_t readExtID();
    void initWiiExt();
    void setEuphoriaLed(bool state);
    I2CMasterInterface mInterface;
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

    bool hiRes = false;
    bool hasTapBar = false;
    WiiExtType_t mType = WII_NO_EXTENSION;
    MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
};