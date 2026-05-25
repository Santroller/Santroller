// Support for band hero drums as midi devices
#pragma once
#include "i2c.hpp"
#include "devices/midi.hpp"
#define DRUM_ADDR 0x0D
#define BH_DRUM_PTR 0x10
#define MAX_PACKETS_IN_BUFFER 0x0F
#define PACKET_SIZE 3
typedef enum
{
    BH_DRUM_CHECK_STATUS,
    BH_DRUM_READ_DATA
} bh_status_e;
class BandHeroDrum : public I2CDMAInterface
{
public:
    BandHeroDrum(MidiDevice *midiDevice, uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock), m_device(midiDevice) {};
    void tick();
    void begin();
    void end();
    void processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return connected;
    }

private:
    I2CMasterInterface interface;
    bool connected;
    long m_lastTick = 0;
    uint8_t m_lastCount = 0;
    MidiDevice *m_device;
    bh_status_e status = BH_DRUM_CHECK_STATUS;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id;
    int failCount = 0;
    uint8_t numPackets;
};
