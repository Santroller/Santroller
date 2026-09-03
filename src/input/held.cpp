#include "input/held.hpp"
#include "utils.h"

HeldInput::HeldInput()
{
}
void HeldInput::load(proto_HeldInput config, std::unique_ptr<Input> input) {
    m_input = std::move(input);
    m_time = config.time;
}
void HeldInput::setup()
{
    m_input->setup();
}
bool HeldInput::tick_digital()
{
    if (m_input->tick_digital())
    {
        if (m_last_pressed == 0)
        {
            m_last_pressed = millis();
        }
        if (millis() - m_last_pressed > m_time)
        {
            return true;
        }
    } else {
        m_last_pressed = 0;
    }
    return false;
}
uint16_t HeldInput::tick_analog()
{
    return tick_digital() ? 65535 : 0;
}