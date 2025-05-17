#pragma once

#include "i2c.hpp"
#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12
class DJHeroTurntable {
   public:
   DJHeroTurntable(I2CMasterInterface* interface, bool left)
        : interface(interface), addr(left ? DJLEFT_ADDR: DJRIGHT_ADDR) {};
    void tick();
    inline bool isConnected() {
        return connected;
    }

   private:
    I2CMasterInterface* interface;
    uint8_t addr;
    bool connected;
    int pollRate;
    uint8_t velocity;
    bool green;
    bool red;
    bool blue;
};
