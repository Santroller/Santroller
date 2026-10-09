#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/joybus.hpp"
#include <memory>
#include "profiles/profile.hpp"
class JoybusAxisInput : public Input
{
public:
    JoybusAxisInput(proto_JoybusAxisInput input, std::shared_ptr<JoybusDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool valid() const override { return m_device != nullptr && m_device->valid(); }

private:
    void setup();
    proto_JoybusAxisInput m_input;
    std::shared_ptr<JoybusDevice> m_device;
};
class JoybusButtonInput : public Input
{
public:
    JoybusButtonInput(proto_JoybusButtonInput input, std::shared_ptr<JoybusDevice> device, Profile *profile);
    bool tick_digital();
    uint16_t tick_analog();
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_JoybusButton) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.button); }

private:
    void setup();
    proto_JoybusButtonInput m_input;
    std::shared_ptr<JoybusDevice> m_device;
};
