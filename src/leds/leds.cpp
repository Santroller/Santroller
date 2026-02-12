#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
void InputLedMapping::update()
{
    m_device->set_val(m_input->tickAnalog());
}
void PatternLedMapping::update()
{
}
void StaticLedMapping::update()
{
}
void RgbLedDevice::set_val(uint16_t val)
{
    uint16_t r = map(val, 0, UINT16_MAX, m_device.startR, m_device.endR);
    uint16_t g = map(val, 0, UINT16_MAX, m_device.startG, m_device.endG);
    uint16_t b = map(val, 0, UINT16_MAX, m_device.startB, m_device.endB);
    m_led_device->set_led(m_device.activeLed, r, g, b);
}
void STP16CPCLedDevice::set_val(uint16_t val)
{
    m_led_device->set_led(m_device.activeLed, val, val, val);
}
void GpioLedDevice::set_val(uint16_t val)
{
    pwm_set_gpio_level(m_device.pin, val);
}
void RgbLedDevice::setup()
{
}
void STP16CPCLedDevice::setup()
{
}
void GpioLedDevice::setup()
{
    if (m_device.analog)
    {
        gpio_set_function(m_device.pin, GPIO_FUNC_PWM);
        pwm_set_gpio_level(m_device.pin, 0);
    }
    else
    {
        gpio_init(m_device.pin);
        gpio_set_dir(m_device.pin, true);
    }
}
void InputLedMapping::reload() {
    m_device->setup();
    m_input->setup();
}
void PatternLedMapping::reload() {
    m_device->setup();
}
void StaticLedMapping::reload() {
    m_device->setup();
}