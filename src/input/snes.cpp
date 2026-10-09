#include "input/snes.hpp"

SNESAxisInput::SNESAxisInput(proto_SNESAxisInput input, std::shared_ptr<SNESDevice> device, Profile *profile) : m_input(input), m_device(device)
{
}
bool SNESAxisInput::tick_digital()
{
    return m_device && m_device->read_axis(m_input.axis) > 0;
}
uint16_t SNESAxisInput::tick_analog()
{
    return m_device ? m_device->read_axis(m_input.axis) : 0;
}
void SNESAxisInput::setup()
{
}
SNESButtonInput::SNESButtonInput(proto_SNESButtonInput input, std::shared_ptr<SNESDevice> device, Profile *profile) : m_input(input), m_device(device)
{
}
bool SNESButtonInput::tick_digital()
{
    return m_device && m_device->read_button(m_input.button);
}
uint16_t SNESButtonInput::tick_analog()
{
    return tick_digital() ? UINT16_MAX : 0;
}
void SNESButtonInput::setup()
{
}
