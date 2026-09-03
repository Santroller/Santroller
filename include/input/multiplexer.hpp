#include "input.hpp"
#include "input.pb.h"
#include "devices/multiplexer.hpp"
#include <memory>
#pragma once
class MultiplexerInput: public Input {
   public:
    MultiplexerInput(proto_MultiplexerInput input, std::shared_ptr<MultiplexerDevice> device);
    bool tick_digital();
    uint16_t tick_analog();
   private:
    void setup();
    uint8_t m_channel;
    proto_MultiplexerInput m_input;
    std::shared_ptr<MultiplexerDevice> m_device;
};