#include "input.hpp"
#include "input.pb.h"
#include <vector>
#include <memory>
#pragma once
class ShortcutInput: public Input {
   public:
    ShortcutInput();
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();
    std::vector<std::unique_ptr<Input>> inputs;
};