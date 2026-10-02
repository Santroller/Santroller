#pragma once

#include <memory>
#include "input/input.hpp"
#include "devices/secondary_pico.hpp"

class Profile;

class PeripheralInput : public Input
{
public:
    PeripheralInput(proto_PeripheralInput input, std::shared_ptr<SecondaryPicoDevice> device, Profile *profile);
    bool tick_digital() override;
    uint16_t tick_analog() override;
    void setup() override {}
    bool has_independent_analog_value() const override { return m_input.analog; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_Peripheral) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.pin); }

private:
    proto_PeripheralInput m_input;
    std::shared_ptr<SecondaryPicoDevice> m_device;
};
