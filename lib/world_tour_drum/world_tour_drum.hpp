#pragma once
#include "devices/midi.hpp"
#include <stdint.h>
#include "spi.hpp"
typedef enum
{
    WT_DRUM_REQUEST_STATUS,
    WT_DRUM_CHECK_STATUS,
    WT_DRUM_READ_COUNT,
    WT_DRUM_READ_DATA,
} wt_status_e;
class WorldTourDrum
{
public:
    ~WorldTourDrum(){};
    WorldTourDrum(MidiDevice *midiDevice, int8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, int8_t csPin);
    void tick();
    void process_data();
    void begin();
    void end();
    inline bool is_connected()
    {
        return connected;
    }

private:
    void schedule(uint32_t delay_us);
    SPIMasterInterface mInterface;
    int8_t mCsPin;
    int missing = 0;
    bool connected = false;
    MidiDevice *m_device;
    uint32_t lastPoll = 0;
    wt_status_e status = WT_DRUM_REQUEST_STATUS;
    alarm_id_t restart_alarm_id = 0;
    volatile bool finished = false;
    uint8_t m_rxBuf[3] = {0};
    uint8_t m_pos = 0;
    uint8_t m_len = 1;
};