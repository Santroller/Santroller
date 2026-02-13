#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "usb/device/hid_device.h"
void InputLedMapping::update()
{
    uint16_t raw = m_input->tickAnalog();
    uint16_t curr = map_16(raw, m_mapping.min, m_mapping.max, 0, UINT16_MAX);
    if ((raw != m_last_val || m_resend) && (millis() - m_last_poll) > 10 && !HIDConfigDevice::tool_closed())
    {
        m_last_val = raw;
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, raw, curr}}};
        m_resend = !HIDConfigDevice::send_event_for(event, m_profile_id);
        if (!m_resend)
        {
            m_last_poll = millis();
        }
    }
    m_device->set_val(curr);
}
void PatternLedMapping::update()
{
    if (millis() < m_next_poll)
    {
        return;
    }
    uint8_t m_leds = m_device->led_count();
    uint8_t m_brightness = 80;
    uint8_t section = m_brightness / 3;
    uint8_t section2 = section * 2;
    if (m_mapping.pattern == PatternRainbow)
    {
        for (int i = 0; i < m_leds; i++)
        {
            auto pos = (i * m_brightness / m_leds + m_pos) % m_brightness;
            if (pos < section)
            {
                auto g = 0;
                auto r = pos;
                auto b = m_brightness - r;
                m_device->set_val_raw(i, r, g, b);
            }
            else if (pos < section2)
            {
                auto g = pos - section;
                auto r = m_brightness - g;
                auto b = 0;
                m_device->set_val_raw(i, r, g, b);
            }
            else if (pos)
            {
                auto b = pos - section2;
                auto g = m_brightness - b;
                auto r = 0;
                m_device->set_val_raw(i, r, g, b);
            }
        }
        m_pos++;
    }
    else if (m_mapping.pattern == PatternFade)
    {
        m_device->set_val(m_pos);
        if (m_dir)
        {
            m_pos-=127;
        }
        else
        {
            m_pos+=127;
        }
        if (m_pos >= UINT16_MAX && !m_dir)
        {
            m_dir = true;
            m_pos-=127;
        }
        if (m_pos == 0 && m_dir)
        {
            m_dir = false;
        }
    }
    m_next_poll = millis() + m_speed;
}
void StaticLedMapping::update()
{
    m_device->set_val(UINT16_MAX);
}
void RgbLedDevice::set_val(uint16_t val)
{
    uint16_t r = map_16(val, 0, UINT16_MAX, m_device.startR, m_device.endR);
    uint16_t g = map_16(val, 0, UINT16_MAX, m_device.startG, m_device.endG);
    uint16_t b = map_16(val, 0, UINT16_MAX, m_device.startB, m_device.endB);
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], r, g, b);
    }
}
void RgbLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b);
}
uint8_t RgbLedDevice::led_count()
{
    return m_device.activeLed_count;
}
void STP16CPCLedDevice::set_val(uint16_t val)
{
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], val, val, val);
    }
}
void STP16CPCLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b);
}
uint8_t STP16CPCLedDevice::led_count()
{
    return m_device.activeLed_count;
}
void GpioLedDevice::set_val(uint16_t val)
{
    pwm_set_gpio_level(m_device.pin, val);
}
void GpioLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b)
{
    pwm_set_gpio_level(m_device.pin, r);
}
uint8_t GpioLedDevice::led_count()
{
    return 1;
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