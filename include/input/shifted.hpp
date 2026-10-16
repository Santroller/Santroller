#pragma once
#include "input.hpp"
#include "input.pb.h"
#include <memory>

class ShiftedInput : public Input
{
public:
    ShiftedInput();
    void load(proto_ShiftedInput config, std::unique_ptr<Input> input, std::unique_ptr<Input> shift);
    bool tick_digital() override;
    uint16_t tick_analog() override;
    bool has_independent_analog_value() const override { return m_input ? m_input->has_independent_analog_value() : false; }
    void setup() override;
    ShortcutInput* as_shortcut() override { return nullptr; }
    uint64_t hardware_id() const override { return m_input ? m_input->hardware_id() : 0; }
    Input* get_inner_input() const { return m_input.get(); }
    Input* get_shift_input() const { return m_shift.get(); }
    bool valid() const override { return (m_input && m_input->valid()) && (m_shift && m_shift->valid()); }
    void link_device(bool claim_devices) override
    {
        if (m_input) m_input->link_device(claim_devices);
        if (m_shift) m_shift->link_device(claim_devices);
    }
    uint32_t linked_device_serial() const override { return mix_serial(mix_serial(0, m_input.get()), m_shift.get()); }

private:
    std::unique_ptr<Input> m_input;
    std::unique_ptr<Input> m_shift;
    bool m_invert_shift = false;
};
