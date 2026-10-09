#include "input.hpp"
#include "input.pb.h"
#include "devices/cycle.hpp"
#include <vector>
#include <memory>
#pragma once
class CycleInput : public Input
{
public:
    CycleInput();
    void load(proto_CycleInput config, std::shared_ptr<CycleDevice> device, std::unique_ptr<Input> input, std::unique_ptr<Input> input_reverse);
    bool tick_digital();
    uint16_t tick_analog();
    bool has_independent_analog_value() const override { return m_input ? m_input->has_independent_analog_value() : false; }
    void setup();
    uint64_t hardware_id() const override { return m_input ? m_input->hardware_id() : m_input_reverse ? m_input_reverse->hardware_id() : 0; }
    bool valid() const override { return m_input ? m_input->valid() : m_input_reverse ? m_input_reverse->valid() : false; }

private:
    std::unique_ptr<Input> m_input;
    std::unique_ptr<Input> m_input_reverse;
    uint32_t m_last_toggled = 0;
    bool m_last_state = false;
    bool m_last_state_reverse = false;
    std::shared_ptr<CycleDevice> m_device;
};