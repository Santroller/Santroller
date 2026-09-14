#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/wii.hpp"
#include <memory>
class WiiAxisInput : public Input
{
public:
    WiiAxisInput(proto_WiiAxisInput input, std::shared_ptr<WiiDevice> device);
    bool tick_digital();
    uint16_t tick_analog();

private:
    void setup();
    proto_WiiAxisInput m_input;
    std::shared_ptr<WiiDevice> m_device;
};
class WiiButtonInput : public Input
{
public:
    WiiButtonInput(proto_WiiButtonInput input, std::shared_ptr<WiiDevice> device);
    bool tick_digital();
    uint16_t tick_analog();
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_WiiButton) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_WiiButtonInput m_input;
    std::shared_ptr<WiiDevice> m_device;
};