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
bool HeldInput::tickDigital()
{
    if (m_input->tickDigital())
    {
        if (m_lastPressed == 0)
        {
            m_lastPressed = millis();
        }
        if (millis() - m_lastPressed > m_time)
        {
            return true;
        }
    } else {
        m_lastPressed = 0;
    }
    return false;
}
uint16_t HeldInput::tickAnalog()
{
    return tickDigital() ? 65535 : 0;
}