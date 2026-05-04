#pragma once
#include <stdint.h>
#include "i2c.hpp"
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "devices/midi.hpp"
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
    WII_INIT_FINISH_ENC,
    WII_INIT_FB_0,
    WII_INIT_READ_ID_WRITE_PTR,
    WII_INIT_READ_ID_READ,
    WII_INIT_DRAWSOME,
    WII_INIT_CLASSIC_0,
    WII_INIT_CLASSIC_1,
    WII_INIT_CLASSIC_2,
    WII_INIT_CLASSIC_READ_ID_WRITE_PTR,
    WII_INIT_CLASSIC_READ_ID_READ,
    WII_INIT_READ_DATA_WRITE_PTR,
    WII_INIT_READ_DATA_READ,
    WII_INIT_ENABLE_ENC_0,
    WII_INIT_ENABLE_ENC_1,
    WII_INIT_ENABLE_ENC_2,
    WII_INIT_ENABLE_ENC_3,
    WII_INIT_ENC_READ_ID_WRITE_PTR,
    WII_INIT_ENC_READ_ID_READ,
    WII_INPUTS_WRITE_PTR,
    WII_INPUTS_READ,
    WII_INPUTS_UPDATE_LED
} wii_status_e;
class WiiExtension: public I2CDMAInterface
{

public:
    WiiExtension(MidiDevice* midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock);
    ~WiiExtension();
    void tick();
    void processData(bool running, bool timeout, bool abort_detected, bool stop_detected);
    WiiExtType mType = WiiExtType::WiiNoExtension;
    uint16_t readAxis(proto_WiiAxisType type);
    bool readButton(proto_WiiButtonType type);
    uint8_t mBuffer[8];

private:
    bool verifyData(const uint8_t *dataIn, uint8_t dataSize);
    void setEuphoriaLed(bool state);
    I2CMasterInterface mInterface;
    bool mFound;
    bool nextEuphoriaLedState = false;
    bool ledUpdated = false;
    bool hadDrum = false;
    uint8_t packetIssueCount;
    uint8_t mBufferIndex;
    long lastTick;
    uint8_t wiiBytes;
    uint8_t wiiPointer = 0;
    uint8_t s_box = 0;
    uint8_t m_block = 0;
    MidiDevice *m_device;
    alarm_id_t restart_alarm_id;
    int failCount = 0;

    bool started = false;

    bool hiRes = false;
    bool hasTapBar = false;

    wii_status_e status = WII_INIT_FINISH_ENC;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
};