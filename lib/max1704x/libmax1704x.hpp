#pragma once

#include "i2c.hpp"
#define MAX710X_I2C_ADDRESS 0x36

#define REGISTER_VCELL 0x02
#define REGISTER_SOC 0x04
#define REGISTER_MODE 0x06
#define REGISTER_VERSION 0x08
#define REGISTER_CONFIG 0x0C
#define REGISTER_COMMAND 0xFE

typedef enum
{
    MAX710X_DETECT,
    MAX710X_POLL
} max1704x_status_e;
// MAX17043 / MAX17044 / MAX17048 / MAX17049 fuel gauge. These work out the state of charge
// themselves, so this just reads it.
class Max1704X : public I2CDMAInterface
{
public:
    Max1704X(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    void begin();
    void end();
    void process_data(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return connected;
    }
    // Battery percentage, 0 to 100
    volatile uint8_t batteryLevel = 0;

private:
    void schedule(uint32_t ms);
    I2CMasterInterface interface;
    max1704x_status_e status = MAX710X_DETECT;
    volatile bool connected = false;
    uint8_t bufferTx[4];
    uint8_t bufferRx[4];
    alarm_id_t restart_alarm_id = 0;
};
