#pragma once

#include "i2c.hpp"

#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12
#define GH5_POLL_INTERVAL_US 4000
#define GH5_STARTUP_MS 350

class GuitarHero5Neck : public I2CDMAInterface
{
public:
    GuitarHero5Neck(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    void begin();
    void end();
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return connected;
    }
    bool green = false;
    bool red = false;
    bool yellow = false;
    bool blue = false;
    bool orange = false;
    bool tapGreen = false;
    bool tapRed = false;
    bool tapYellow = false;
    bool tapBlue = false;
    bool tapOrange = false;

private:
    void clear_buttons();
    I2CMasterInterface interface;
    bool connected = false;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id = 0;
    int failCount = 0;
    uint32_t lastPoll = 0;
};
