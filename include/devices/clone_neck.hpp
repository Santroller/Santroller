#pragma once

#include "interfaces/i2c.hpp"
#include "state/base.hpp"

#define CLONE_ADDR 0x10
#define CLONE_VALID_PACKET 0x52

class CloneNeckDevice {
   public:
    inline CloneNeckDevice(I2CMasterInterface* interface) : mInterface(interface), mFound(false) {
    }
    void tick(san_base_t* data);

   private:
    I2CMasterInterface* mInterface;
    bool mFound;
    bool reading;
    bool clone_ready;
    long clone_guitar_timer = 0;
    long clone_guitar_ready_timer = 0;
};