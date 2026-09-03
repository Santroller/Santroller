#include "input/ps2.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

PS2AxisInput::PS2AxisInput(proto_PS2AxisInput input, std::shared_ptr<PS2Device> device) : m_input(input), m_device(device)
{
}
bool PS2AxisInput::tick_digital()
{
    return m_device->read_axis(m_input.axis) > 0;
}
uint16_t PS2AxisInput::tick_analog()
{
    return m_device->read_axis(m_input.axis);
}
void PS2AxisInput::setup()
{
}
PS2ButtonInput::PS2ButtonInput(proto_PS2ButtonInput input, std::shared_ptr<PS2Device> device) : m_input(input), m_device(device)
{
}
bool PS2ButtonInput::tick_digital()
{
    return m_device->read_button(m_input.button);
}
uint16_t PS2ButtonInput::tick_analog()
{
    return m_device->read_button(m_input.button) ? 65535 : 0;
}
void PS2ButtonInput::setup()
{
}
