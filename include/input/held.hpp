#include "input.hpp"
#include "input.pb.h"
#include <vector>
#include <memory>
#pragma once
class HeldInput : public Input
{
public:
    HeldInput();
    void load(proto_HeldInput config, std::unique_ptr<Input> input);
    bool tick_digital();
    uint16_t tick_analog();
    void setup();
    ShortcutInput* as_shortcut() override { return m_input ? m_input->as_shortcut() : nullptr; }
    uint64_t hardware_id() const override { return m_input ? m_input->hardware_id() : 0; }
    Input* get_inner_input() const { return m_input.get(); }

private:
    std::unique_ptr<Input> m_input;
    uint64_t m_last_pressed = 0;
    uint64_t m_time = 0;
};