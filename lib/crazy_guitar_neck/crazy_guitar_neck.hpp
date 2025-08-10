#pragma once

#include "i2c.hpp"
#define CLONE_ADDR 0x10
#define CLONE_VALID_PACKET 0x52
class CrazyGuitarNeck {
   public:
    CrazyGuitarNeck(uint8_t block, uint8_t sda, uint8_t scl, uint32_t clock)
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
    bool soloGreen;
    bool soloRed;
    bool soloYellow;
    bool soloBlue;
    bool soloOrange;

   private:
    I2CMasterInterface interface;
    bool connected;
    bool reading;
};
