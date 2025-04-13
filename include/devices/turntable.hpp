#pragma once

#include "interfaces/i2c.hpp"
#include "state/base.hpp"

#define DJLEFT_ADDR 0x0E
#define DJRIGHT_ADDR 0x0D
#define DJ_BUTTONS_PTR 0x12

class TurntableDevice {
   public:
    inline TurntableDevice(I2CMasterInterface* interface, int pollRate) : mInterface(interface), mFound(false), mPollRate(pollRate) {
    }
    void tick(san_base_t* data);

   private:
    I2CMasterInterface* mInterface;
    bool mFound;
    int mPollRate;
    long lastDj;
    bool mLeftConnected;
    bool mRightConnected;
};