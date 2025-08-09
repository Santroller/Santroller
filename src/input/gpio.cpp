#include "input/gpio.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

GPIOInput::GPIOInput(proto_GPIOInput input) : m_analog(input.analog), m_input(input)
{
    setup();
}
bool GPIOInput::tickDigital()
{
    return gpio_get(m_pin);
}
uint16_t GPIOInput::tickAnalog()
{
    adc_select_input(m_pin - ADC_BASE_PIN);
    return adc_read() << 4;
}
void GPIOInput::setup()
{
    m_pin = m_input.pin;
    if (m_analog)
    {
        adc_gpio_init(m_pin);
    } else {
        gpio_init(m_pin);
    }
    switch (m_input.pinMode)
    {
    case PinMode::BusKeep:
        gpio_set_pulls(m_pin, true, true);
        break;
    case PinMode::Floating:
        gpio_set_pulls(m_pin, false, false);
        break;
    case PinMode::PullDown:
        gpio_set_pulls(m_pin, false, true);
        break;
    case PinMode::PullUp:
        gpio_set_pulls(m_pin, true, false);
        break;
    }
}