#pragma once

#include <memory>
#include "input/input.hpp"
#include "devices/secondary_pico.hpp"

class Profile;

// A pin on the secondary pico itself
class PeripheralInput : public Input
{
public:
    PeripheralInput(proto_PeripheralInput input, std::shared_ptr<SecondaryPicoDevice> device, Profile *profile);
    ~PeripheralInput();
    bool tick_digital() override;
    uint16_t tick_analog() override;
    void setup() override {}
    bool has_independent_analog_value() const override { return m_input.analog; }
    bool valid() const override { return m_device != nullptr && m_device->valid(); }
    uint64_t hardware_id() const override { return (static_cast<uint64_t>(InputHw_Peripheral) << 56) | (static_cast<uint64_t>(m_input.deviceid) << 16) | static_cast<uint32_t>(m_input.pin); }

private:
    proto_PeripheralInput m_input;
    std::shared_ptr<SecondaryPicoDevice> m_device;
    // What the secondary pico reports this input as
    uint32_t m_id = 0;
};

// An input on a device running on a secondary pico. The secondary pico reads it and sends back its
// state. The input as it would be built here (without its device) answers what it is
class SecondaryPicoForwardedInput : public Input
{
public:
    SecondaryPicoForwardedInput(const proto_Input &input, std::shared_ptr<SecondaryPicoDevice> device, std::unique_ptr<Input> local);
    ~SecondaryPicoForwardedInput();
    bool tick_digital() override;
    uint16_t tick_analog() override;
    void setup() override {}
    bool has_independent_analog_value() const override { return m_local && m_local->has_independent_analog_value(); }
    bool valid() const override { return m_id && m_device->valid(); }
    uint64_t hardware_id() const override { return m_local ? m_local->hardware_id() : 0; }

private:
    std::shared_ptr<SecondaryPicoDevice> m_device;
    std::unique_ptr<Input> m_local;
    uint32_t m_id = 0;
};
