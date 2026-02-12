#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "usb/device/hid_device.h"
void InputLedMapping::update()
{
    uint16_t curr = map(m_input->tickAnalog(), m_mapping.min, m_mapping.max, 0, UINT16_MAX);
    if ((curr != m_last_val || m_resend) && !HIDConfigDevice::tool_closed())
    {
        m_last_val = curr;
        proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, curr, curr}}};
        m_resend = !HIDConfigDevice::send_event_for(event, m_profile_id);
    }
    m_device->set_val(curr);
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
void InputLedMapping::reload()
{
    m_device->setup();
    m_input->setup();
}
void PatternLedMapping::reload()
{
    m_device->setup();
}
void StaticLedMapping::reload()
{
    m_device->setup();
}