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
    uint64_t hardware_id() const override {
        return (static_cast<uint64_t>(InputHw_Multiplexer) << 56) |
               (static_cast<uint64_t>(m_device ? m_device->m_id : 0) << 16) |
               static_cast<uint64_t>(m_channel);
    }
   private:
    void setup();
    uint8_t m_channel;
    proto_MultiplexerInput m_input;
    std::shared_ptr<MultiplexerDevice> m_device;
};