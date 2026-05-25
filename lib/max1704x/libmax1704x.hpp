#pragma once

#include "i2c.hpp"
#define MAX710X_I2C_ADDRESS 0x36

#define REGISTER_VCELL 0x02
#define REGISTER_SOC 0x04
#define REGISTER_MODE 0x06
#define REGISTER_VERSION 0x08
#define REGISTER_CONFIG 0x0C
#define REGISTER_COMMAND 0xFE

#define RESET_COMMAND 0x5400
#define QUICKSTART_MODE 0x4000
typedef enum
{
    MAX710X_RESET,
    MAX710X_CHECK_CONFIG,
    MAX710X_QUICKSTART,
    MAX710X_POLL
} max1704x_status_e;
class Max1704X : public I2CDMAInterface
{
public:
    Max1704X(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    void begin();
    void end();
    void processData(uint8_t addr, bool running, bool timeout, bool abort_detected, bool stop_detected);
    inline bool is_connected()
    {
        return status != MAX710X_RESET;
    }

private:
    bool init();
    I2CMasterInterface interface;
    max1704x_status_e status = MAX710X_RESET;
    uint8_t bufferTx[32];
    uint8_t bufferRx[32];
    alarm_id_t restart_alarm_id;
    int failCount = 0;
    uint8_t batteryLevel;
};
