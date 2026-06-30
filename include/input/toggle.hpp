#include "input.hpp"
#include "input.pb.h"
#include "devices/toggle.hpp"
#include <vector>
#include <memory>
#pragma once
class ToggleInput : public Input
{
public:
    ToggleInput();
    void load(proto_ToggleInput config, std::shared_ptr<ToggleDevice> device, std::unique_ptr<Input> input);
    bool tickDigital();
    uint16_t tickAnalog();
    void setup();

private:
    std::unique_ptr<Input> m_input;
    std::unique_ptr<Input> m_input_reverse;
    uint32_t m_last_toggled = 0;
    bool m_last_state = false;
    bool m_last_state_reverse = false;
    std::shared_ptr<ToggleDevice> m_device;
};