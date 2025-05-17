#pragma once

#include "i2c.hpp"
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12
class GuitarHero5Neck {
   public:
    GuitarHero5Neck(I2CMasterInterface* interface)
        : interface(interface) {};
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
    I2CMasterInterface* interface;
    bool connected;
};
