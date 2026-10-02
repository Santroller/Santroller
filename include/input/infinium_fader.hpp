#pragma once

#include <memory>
#include "input/input.hpp"
#include "devices/infinium_fader.hpp"
#include "profiles/profile.hpp"

class InfiniumFaderInput : public Input
{
public:
    InfiniumFaderInput(proto_InfiniumFaderInput input, std::shared_ptr<InfiniumFaderDevice> device, Profile *profile);
    bool tick_digital() override;
    uint16_t tick_analog() override;
    void setup() override {}
    bool has_independent_analog_value() const override { return true; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_InfiniumFader) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16); }

private:
    proto_InfiniumFaderInput m_input;
    std::shared_ptr<InfiniumFaderDevice> m_device;
};
