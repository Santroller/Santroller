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
    void load(proto_CycleInput config, std::shared_ptr<CycleDevice> device, std::unique_ptr<Input> input);
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();

private:
    std::unique_ptr<Input> m_input;
    uint32_t m_last_toggled = 0;
    std::shared_ptr<CycleDevice> m_device;
};