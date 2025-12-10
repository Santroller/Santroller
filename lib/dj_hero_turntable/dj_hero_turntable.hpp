#pragma once

#include "i2c.hpp"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x10
class DJHeroTurntable {
   public:
   DJHeroTurntable(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock, bool left)
        : interface(block, sda, scl, clock), addr(left ? DJLEFT_ADDR: DJRIGHT_ADDR) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }

   private:
    I2CMasterInterface interface;
    uint8_t addr;
    bool connected;
    int pollRate;
    uint8_t velocity;
    bool green;
    bool red;
    bool blue;
};
