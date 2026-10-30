#include "input/gpio.hpp"
#include "devices/pins.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

GPIOInput::GPIOInput(proto_GPIOInput input) : m_analog(input.analog), m_invert(input.pinMode == PinMode_PullUp), m_input(input)
{
    setup();
}
bool GPIOInput::tick_digital()
{
    if (!m_valid)
    {
        return 0;
    }
    return gpio_get(m_pin) != m_invert;
}
uint16_t GPIOInput::tick_analog()
{
    if (!m_valid)
    {
        return 0;
    }
    if (!m_analog)
    {
        return (gpio_get(m_pin) != m_invert) ? 65535 : 0;
    }
    adc_select_input(m_pin - ADC_BASE_PIN);
    return adc_read() << 4;
}
void GPIOInput::setup()
{
    m_pin = m_input.pin;
    // On a Pico W the wireless chip uses some pins (GP29 is its SPI clock), so they can't be read
    // while bluetooth is running
    if (m_input.pin < 0 || BluetoothStack::instance().using_cyw43_pin(m_pin))
    {
        m_valid = false;
        return;
    }
    m_valid = true;
    if (m_analog && is_adc_pin(m_pin))
    {
        adc_gpio_init(m_pin);
    }
    else
    {
        m_analog = false;
        gpio_init(m_pin);
    }
    gpio_set_pin_mode(m_pin, m_input.pinMode);
}