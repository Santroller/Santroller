#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "usb/device/hid_device.h"
void InputLedMapping::update()
{
    uint16_t raw = m_input->tickAnalog();
    uint16_t curr = map_16(raw, m_mapping.min, m_mapping.max, 0, UINT16_MAX);
    if ((curr != m_last_val || m_resend) && (millis() - m_last_poll) > 10 && !HIDConfigDevice::tool_closed())
    {
        m_last_val = curr;
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
    uint8_t m_leds = m_device->led_count();
    if (m_mapping.pattern == PatternRainbow)
    {
        for (int i = 0; i < m_leds; i++)
        {
            auto pos = (i * 256 / m_leds + m_pos) % 256;
            if (pos < 85)
            {
                auto g = 0;
                auto r = ((float)pos / 85.0f) * 255.0f;
                auto b = 255 - r;
                m_device->set_val_raw(i, r, g, b);
            }
            else if (pos < 170)
            {
                auto g = ((float)(pos - 85) / 85.0f) * 255.0f;
                auto r = 255 - g;
                auto b = 0;
                m_device->set_val_raw(i, r, g, b);
            }
            else if (pos < 256)
            {
                auto b = ((float)(pos - 170) / 85.0f) * 255.0f;
                auto g = 255 - b;
                auto r = 1;
                m_device->set_val_raw(i, r, g, b);
            }
        }
        m_pos++;
    } else if (m_mapping.pattern == PatternFade) {
        m_device->set_val(m_pos);
        if (m_dir) {
            m_pos--;
        } else {
            m_pos++;
        }
        if (m_pos == UINT16_MAX && !m_dir) {
            m_dir = true;
        }
        if (m_pos == 0 && m_dir) {
            m_dir = false;
        }
    }
}
void StaticLedMapping::update()
{
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