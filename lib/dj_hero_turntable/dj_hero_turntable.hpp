#pragma once

#include "i2c.hpp"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x10

typedef enum
{
    DJH_CHECK_STATUS,
    DJH_READ_DATA
} djh_status_e;
class DJHeroTurntable : public I2CDMAInterface
{
public:
    DJHeroTurntable(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock, bool left)
        : interface(block, sda, scl, clock), address(left ? DJLEFT_ADDR : DJRIGHT_ADDR) {};
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
    uint8_t address;
    bool connected;
    int pollRate;
    uint8_t velocity;
    bool green;
    bool red;
    bool blue;
    djh_status_e status = DJH_CHECK_STATUS;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id;
    int failCount = 0;
};
