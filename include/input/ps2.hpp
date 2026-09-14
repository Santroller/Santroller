#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/ps2.hpp"
#include <memory>
class PS2AxisInput : public Input
{
public:
    PS2AxisInput(proto_PS2AxisInput input, std::shared_ptr<PS2Device> device);
    bool tick_digital();
    uint16_t tick_analog();

private:
    void setup();
    proto_PS2AxisInput m_input;
    std::shared_ptr<PS2Device> m_device;
};
class PS2ButtonInput : public Input
{
public:
    PS2ButtonInput(proto_PS2ButtonInput input, std::shared_ptr<PS2Device> device);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_PS2Button) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_PS2ButtonInput m_input;
    std::shared_ptr<PS2Device> m_device;
};
