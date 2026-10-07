#include "input/mpr121.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

MPR121Input::MPR121Input(proto_MPR121Input input, std::shared_ptr<MPR121Device> device, Profile* profile) : m_input(input), m_device(device)
{
    if (m_device && m_input.mode == Digital)
    {
        m_device->use_gpio_input(m_input.pin, m_input.pinMode);
    }
    else if (m_device)
    {
        m_device->use_touch(m_input.pin);
    }
}
void MPR121Input::setup()
{
}
bool MPR121Input::tick_digital()
{
    if (m_input.mode == Digital)
    {
        // pulled up inputs are pressed when they are pulled low, like a normal GPIO
        return m_device->gpio_level(m_input.pin) != (m_input.pinMode == PinMode_PullUp);
    }
    return m_device->m_mpr121.inputs & 1 << m_input.pin;
}
uint16_t MPR121Input::tick_analog()
{
    return tick_digital() ? 65535 : 0;
}