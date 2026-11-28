#pragma once
#include "base.hpp"
#include "device.pb.h"

// One input on its own, as a device, so the config tool can treat it as one (syncing its calibration across
// profiles, naming it by its pin's label). Mappings on it read the input as if they had it themselves.
class SingleInputDevice : public Device
{
public:
    SingleInputDevice(uint16_t id) : Device(id) {}
    void begin() {}
    void end(bool full) {}
    void update(bool full_poll, bool send_events) {}
    // The input comes from a callback while the config is decoded (see load_single_input)
    void set_input(const proto_Input &input)
    {
        m_input = input;
        m_has_input = true;
    }
    bool using_pin(uint8_t pin)
    {
        return m_has_input && m_input.which_input == proto_Input_gpio_tag && m_input.input.gpio.pin == pin;
    }
    // nullptr when it has none
    const proto_Input *input() const { return m_has_input ? &m_input : nullptr; }

private:
    proto_Input m_input = proto_Input_init_zero;
    bool m_has_input = false;
};
