#include "input.hpp"
#include "input.pb.h"
#pragma once
class FixedInput: public Input {
   public:
    FixedInput(proto_FixedInput input);
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();
   private:
    proto_FixedInput m_input;
};