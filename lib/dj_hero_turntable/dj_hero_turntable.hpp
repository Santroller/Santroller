#pragma once

#include "i2c.hpp"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x10
#define DJH_DEFAULT_POLL_INTERVAL_MS 5
// Readings are movement since the last read, so polling too often shrinks them towards nothing
#define DJH_MIN_POLL_INTERVAL_MS 1

typedef enum
{
    DJH_CHECK_STATUS,
    DJH_READ_DATA
} djh_status_e;
class DJHeroTurntable : public I2CDMAInterface
{
public:
    DJHeroTurntable(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock, bool left, uint32_t poll_interval_ms)
        : interface(block, sda, scl, clock), address(left ? DJLEFT_ADDR : DJRIGHT_ADDR), pollIntervalUs((poll_interval_ms < DJH_MIN_POLL_INTERVAL_MS ? DJH_MIN_POLL_INTERVAL_MS : poll_interval_ms) * 1000) {};
    void tick();
    void begin();
    void end();
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return connected;
    }
    // How far the platter moved over the last poll interval
    int8_t velocity = 0;
    bool green = false;
    bool red = false;
    bool blue = false;

private:
    void clear();
    void schedule(uint32_t delay_us);
    I2CMasterInterface interface;
    uint8_t address;
    uint32_t pollIntervalUs;
    bool connected = false;
    djh_status_e status = DJH_CHECK_STATUS;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id = 0;
    int failCount = 0;
    uint32_t lastPoll = 0;
};
