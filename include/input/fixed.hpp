#include "input.hpp"
#include "input.pb.h"
#pragma once
class FixedInput: public Input {
   public:
    FixedInput(proto_FixedInput input);
    bool tick_digital();
    uint16_t tick_analog();
    void setup();
   private:
    proto_FixedInput m_input;
};