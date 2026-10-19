#include "input/wii.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

WiiAxisInput::WiiAxisInput(proto_WiiAxisInput input, std::shared_ptr<WiiDevice> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool WiiAxisInput::tick_digital()
{
    if (!m_device)
        return false;
    if (m_input.axis == WiiAxisClassicLeftTrigger)
        return m_device->read_button(WiiButtonClassicLt);
    if (m_input.axis == WiiAxisClassicRightTrigger)
        return m_device->read_button(WiiButtonClassicRt);
    return m_device->read_axis(m_input.axis) > 0;
}
uint16_t WiiAxisInput::tick_analog()
{
    return m_device ? m_device->read_axis(m_input.axis) : 0;
}
void WiiAxisInput::setup()
{
}
WiiButtonInput::WiiButtonInput(proto_WiiButtonInput input, std::shared_ptr<WiiDevice> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool WiiButtonInput::tick_digital()
{
    return m_device && m_device->read_button(m_input.button) > 0;
}
uint16_t WiiButtonInput::tick_analog()
{
    return m_device ? m_device->read_button_pressure(m_input.button) : 0;
}
void WiiButtonInput::setup()
{
}