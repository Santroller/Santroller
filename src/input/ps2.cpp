#include "input/ps2.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

PS2AxisInput::PS2AxisInput(proto_PS2AxisInput input, std::shared_ptr<PS2Device> device) : m_input(input), m_device(device)
{
}
bool PS2AxisInput::tickDigital()
{
    return m_device->readAxis(m_input.axis) > 0;
}
uint16_t PS2AxisInput::tickAnalog()
{
    return m_device->readAxis(m_input.axis);
}
void PS2AxisInput::setup()
{
}
PS2ButtonInput::PS2ButtonInput(proto_PS2ButtonInput input, std::shared_ptr<PS2Device> device) : m_input(input), m_device(device)
{
}
bool PS2ButtonInput::tickDigital()
{
    return m_device->readButton(m_input.button);
}
uint16_t PS2ButtonInput::tickAnalog()
{
    return m_device->readButton(m_input.button) ? 65535 : 0;
}
void PS2ButtonInput::setup()
{
}
