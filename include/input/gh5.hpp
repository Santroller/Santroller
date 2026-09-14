#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/gh5neck.hpp"
#include <memory>
class Gh5ButtonInput : public Input
{
public:
    Gh5ButtonInput(proto_Gh5NeckButtonInput input, std::shared_ptr<GH5NeckDevice> device);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_Gh5Neck) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_Gh5NeckButtonInput m_input;
    std::shared_ptr<GH5NeckDevice> m_device;
};