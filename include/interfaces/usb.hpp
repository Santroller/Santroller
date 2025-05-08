#pragma once
#include <stdint.h>
class USBInterface {
   public:
    virtual void sendReport(uint8_t* report, uint16_t len) = 0;
};