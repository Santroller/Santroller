#include "input.hpp"
#include "input.pb.h"
#include <vector>
#include <memory>
#pragma once
class ShortcutInput: public Input {
   public:
    ShortcutInput();
    bool tick_digital();
    uint16_t tick_analog();
    void setup();
    ShortcutInput* as_shortcut() override { return this; }
    const std::vector<std::unique_ptr<Input>>& get_inputs() const { return inputs; }
    std::vector<std::unique_ptr<Input>> inputs;
    bool valid() const override { 
        if (inputs.empty()) return false;
        for (const auto& input : inputs) {
            if (!input->valid()) return false;
        }
        return true;
    }
    void link_device(bool claim_devices) override
    {
        for (const auto &input : inputs) input->link_device(claim_devices);
    }
    uint32_t linked_device_serial() const override
    {
        uint32_t serial = 0;
        for (const auto &input : inputs) serial = mix_serial(serial, input.get());
        return serial;
    }
};