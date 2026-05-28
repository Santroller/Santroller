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
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();

private:
    std::unique_ptr<Input> m_input;
    uint64_t m_lastPressed = 0;
    uint64_t m_time = 0;
};