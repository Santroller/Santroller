#include "input/wii.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

WiiAxisInput::WiiAxisInput(proto_WiiAxisInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiAxisInput::tickDigital()
{
    return m_device->readAxis(m_input.axis) > 0;
}
uint16_t WiiAxisInput::tickAnalog()
{
    return m_device->readAxis(m_input.axis);
}
void WiiAxisInput::setup()
{
}
WiiButtonInput::WiiButtonInput(proto_WiiButtonInput input, std::shared_ptr<WiiDevice> device) : m_input(input), m_device(device)
{
}
bool WiiButtonInput::tickDigital()
{
    return m_device->readButton(m_input.button) > 0;
}
uint16_t WiiButtonInput::tickAnalog()
{
    return m_device->readButton(m_input.button) ? 65535 : 0;
}
void WiiButtonInput::setup()
{
}