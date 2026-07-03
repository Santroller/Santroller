#pragma once
#include "devices/midi.hpp"
#include <stdint.h>
#include "spi.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
typedef enum
{
    WT_DRUM_REQUEST_STATUS,
    WT_DRUM_CHECK_STATUS,
    WT_DRUM_READ_DATA,
    WT_DRUM_END
} wt_status_e;
class WorldTourDrum
{
public:
    WorldTourDrum(MidiDevice *midiDevice, int8_t block, int8_t sck, int8_t mosi, int8_t miso, uint32_t clock, int8_t csPin);
    void tick();
    void processData();
    void begin();
    void end();
    inline bool is_connected()
    {
        return connected;
    }

private:
    SPIMasterInterface mInterface;
    int8_t mCsPin;
    int missing;
    bool connected;
    MidiDevice *m_device;
    uint32_t last = 0;
    wt_status_e status = WT_DRUM_REQUEST_STATUS;
    alarm_id_t restart_alarm_id;
    bool finished = false;
};