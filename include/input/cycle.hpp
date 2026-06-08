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
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();

private:
    std::unique_ptr<Input> m_input;
    std::unique_ptr<Input> m_input_reverse;
    uint32_t m_last_toggled = 0;
    bool m_last_state = false;
    bool m_last_state_reverse = false;
    std::shared_ptr<CycleDevice> m_device;
};