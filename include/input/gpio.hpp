#include "input.hpp"
#include "input.pb.h"
#pragma once
class GPIOInput: public Input {
   public:
    GPIOInput(proto_GPIOInput input);
    bool tickDigital();
    uint16_t tickAnalog();
   private:
    void setup();
    bool m_analog;
    uint8_t m_pin;
    proto_GPIOInput m_input;
};