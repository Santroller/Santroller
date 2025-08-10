#pragma once

#include "i2c.hpp"
#define MAX710X_I2C_ADDRESS 0x36

#define REGISTER_VCELL      0x02
#define REGISTER_SOC        0x04
#define REGISTER_MODE       0x06
#define REGISTER_VERSION    0x08
#define REGISTER_CONFIG     0x0C
#define REGISTER_COMMAND    0xFE

#define RESET_COMMAND       0x5400
#define QUICKSTART_MODE     0x4000
class Max1704X {
   public:
    Max1704X(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
        : interface(block, sda, scl, clock) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }
    bool green;
    bool red;
    bool yellow;
    bool blue;
    bool orange;
    uint8_t slider;

   private:
    inline bool write16(uint8_t addr, uint16_t data) {
        return interface.writeRegister(MAX710X_I2C_ADDRESS, addr, sizeof(data), (uint8_t*)&data);
    }
    inline bool write8(uint8_t addr, uint8_t data) {
        return interface.writeRegister(MAX710X_I2C_ADDRESS, addr, data);
    }
    bool init();
    I2CMasterInterface interface;
    bool connected;
};
