#include "input.hpp"
#include "input.pb.h"
#include "profiles/profile.hpp"
#pragma once
class GPIOInput: public Input {
   public:
    GPIOInput(proto_GPIOInput input);
    bool tick_digital();
    uint16_t tick_analog();
    void setup();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_GPIO) << 56) | m_pin; }
   private:
    bool m_analog;
    bool m_invert;
    bool m_valid;
    uint8_t m_pin;
    proto_GPIOInput m_input;
};