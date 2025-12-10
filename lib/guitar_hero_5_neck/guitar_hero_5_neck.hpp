#pragma once

#include "i2c.hpp"
#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x11
class GuitarHero5Neck {
   public:
    GuitarHero5Neck(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
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
    bool tapGreen;
    bool tapRed;
    bool tapYellow;
    bool tapBlue;
    bool tapOrange;

   private:
    I2CMasterInterface interface;
    bool connected;
};
