#include "input.hpp"
#include "input.pb.h"
#include <vector>
#include <memory>
#pragma once
class ShortcutInput: public Input {
   public:
    ShortcutInput(proto_ShortcutInput input);
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();
    std::vector<std::unique_ptr<Input>> inputs;
   private:
    proto_ShortcutInput m_input;
};