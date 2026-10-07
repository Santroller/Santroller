#pragma once
#include <stdint.h>
#include "i2c.hpp"
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "devices/midi.hpp"
#include "devices/base.hpp"
#include "wii_extension_decoder.hpp"
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

// Minimum poll interval for a DJ Hero turntable when the config does not set one
#define WII_TURNTABLE_DEFAULT_POLL_INTERVAL_MS 5

class WiiExtension: public I2CDMAInterface, public WiiExtensionDecoder
{

public:
    WiiExtension(MidiDevice* midiDevice, uint8_t block, uint8_t sda, uint8_t scl,
                 uint32_t clock, uint32_t turntable_poll_interval_ms);
    ~WiiExtension();
    void begin();
    void end();
    void load_state(const DeviceReloadState *state);
    void save_state(DeviceReloadState& state) const ;
    void tick();
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    void setEuphoriaLed(bool state);

private:
    bool verifyData(const uint8_t *dataIn, uint8_t dataSize);
    I2CMasterInterface mInterface;
    bool mFound;
    bool nextEuphoriaLedState = false;
    bool ledUpdated = false;
    bool hadDrum = false;
    uint8_t packetIssueCount;
    uint8_t mBufferIndex;
    long lastTick;
    uint32_t lastPoll = 0;
    uint8_t wiiBytes;
    uint8_t wiiPointer = 0;
    uint8_t m_block = 0;
    // in us, so the interval is steady rather than jittering by up to a ms
    uint32_t m_turntable_poll_interval_us = 0;
    uint32_t m_last_turntable_poll_us = 0;
    bool m_has_turntable_poll = false;
    MidiDevice *m_device;
    alarm_id_t restart_alarm_id;
    int failCount = 0;

    bool started = false;

    wii_status_e status = WII_INIT_FINISH_ENC;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
};