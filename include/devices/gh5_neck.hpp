#pragma once

#include "interfaces/i2c.hpp"
#include "state/base.hpp"

#define GH5NECK_ADDR 0x0D
#define GH5NECK_BUTTONS_PTR 0x12

class Gh5NeckDevice {
   public:
    inline Gh5NeckDevice(I2CMasterInterface* interface) : mInterface(interface), mFound(false) {
    }
    void tick(san_base_t* data);

   private:
    I2CMasterInterface* mInterface;
    bool mFound;
};