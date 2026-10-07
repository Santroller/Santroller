#pragma once

#include "i2c.hpp"
#define CLONE_ADDR 0x10
#define CLONE_VALID_PACKET 0x52
typedef enum
{
    CLONE_NECK_CHECK_STATUS,
    CLONE_NECK_READ_DATA
} clone_status_e;
class CrazyGuitarNeck : public I2CDMAInterface
{
public:
    CrazyGuitarNeck(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
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
    bool soloGreen = false;
    bool soloRed = false;
    bool soloYellow = false;
    bool soloBlue = false;
    bool soloOrange = false;

private:
    void clear_buttons();
    I2CMasterInterface interface;
    bool connected = false;
    clone_status_e status = CLONE_NECK_CHECK_STATUS;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id = 0;
    int failCount = 0;
    uint32_t lastPoll = 0;
};
