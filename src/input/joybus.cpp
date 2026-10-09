#include "input/joybus.hpp"

JoybusAxisInput::JoybusAxisInput(proto_JoybusAxisInput input, std::shared_ptr<JoybusDevice> device, Profile *profile) : m_input(input), m_device(device)
{
}
bool JoybusAxisInput::tick_digital()
{
    return m_device && m_device->read_axis(m_input.axis) > 0;
}
uint16_t JoybusAxisInput::tick_analog()
{
    return m_device ? m_device->read_axis(m_input.axis) : 0;
}
void JoybusAxisInput::setup()
{
}
JoybusButtonInput::JoybusButtonInput(proto_JoybusButtonInput input, std::shared_ptr<JoybusDevice> device, Profile *profile) : m_input(input), m_device(device)
{
}
bool JoybusButtonInput::tick_digital()
{
    return m_device && m_device->read_button(m_input.button);
}
uint16_t JoybusButtonInput::tick_analog()
{
    return tick_digital() ? UINT16_MAX : 0;
}
void JoybusButtonInput::setup()
{
}
