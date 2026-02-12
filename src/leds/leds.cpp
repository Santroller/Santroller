#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "usb/device/hid_device.h"
void InputLedMapping::update()
{
    uint16_t raw = m_input->tickAnalog();
    uint16_t curr = map_16(raw, m_mapping.min, m_mapping.max, 0, UINT16_MAX);
    if ((curr != m_last_val || m_resend) && !HIDConfigDevice::tool_closed())
    {
        m_last_val = curr;
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, raw, curr}}};
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
    uint16_t r = map_16(val, 0, UINT16_MAX, m_device.startR, m_device.endR);
    uint16_t g = map_16(val, 0, UINT16_MAX, m_device.startG, m_device.endG);
    uint16_t b = map_16(val, 0, UINT16_MAX, m_device.startB, m_device.endB);
    for (int i = 0; i < m_device.activeLed_count; i++) {
        m_led_device->set_led(m_device.activeLed[i], r, g, b);
    }
}
void STP16CPCLedDevice::set_val(uint16_t val)
{
    for (int i = 0; i < m_device.activeLed_count; i++) {
        m_led_device->set_led(m_device.activeLed[i],  val, val, val);
    }
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