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
};