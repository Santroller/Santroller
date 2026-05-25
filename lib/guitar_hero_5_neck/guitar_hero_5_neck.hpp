#pragma once

#include "i2c.hpp"
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x11
typedef enum
{
    GH5_NECK_CHECK_STATUS,
    GH5_NECK_READ_DATA
} gh5_status_e;
class GuitarHero5Neck : public I2CDMAInterface
{
public:
    GuitarHero5Neck(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    void begin();
    void end();
    void processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return connected;
    }
    bool green;
    bool red;
    bool yellow;
    bool blue;
    bool orange;
    bool tapGreen;
    bool tapRed;
    bool tapYellow;
    bool tapBlue;
    bool tapOrange;

private:
    I2CMasterInterface interface;
    bool connected;
    long m_lastTick;
    gh5_status_e status = GH5_NECK_CHECK_STATUS;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id;
    int failCount = 0;
};
