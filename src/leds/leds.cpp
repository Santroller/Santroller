#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "usb/device/hid_device.h"
void InputLedMapping::update(bool full_poll, bool send_events)
{
    uint16_t raw = m_input->tickAnalog();
    uint16_t curr = map_16(raw, m_mapping.min, m_mapping.max, 0, UINT16_MAX);
    if (send_events && full_poll || (raw != m_last_val || m_resend) && (millis() - m_last_poll) > 10)
    {
        m_last_val = raw;
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, raw, curr}}};
        m_resend = !HIDConfigDevice::send_event(event);
        if (!m_resend)
        {
            m_last_poll = millis();
        }
    }
    m_device->set_val(curr);
}
PatternLedMapping::PatternLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PatternLedMapping mapping, uint32_t profile_id, uint32_t id) : LedMapping(std::move(device), profile_id, id), m_mapping(mapping), m_speed(mapping.speed ? mapping.speed : 1), m_brightness(mapping.brightness ? mapping.brightness : 1)
{
    m_speed = 21 - m_speed;
}
void PatternLedMapping::update(bool full_poll, bool send_events)
{
    if (millis() < m_next_poll)
    {
        return;
    }
    uint8_t m_leds = m_device->led_count();
    uint8_t brightness = m_device->supports_brightness() ? 255 : m_brightness;
    if (m_mapping.pattern == PatternRainbow)
    {
        uint8_t section = brightness / 3;
        uint8_t section2 = section * 2;
        for (int i = 0; i < m_leds; i++)
        {
            auto pos = (i * brightness / m_leds + m_pos) % brightness;
            if (pos < section)
            {
                auto g = 0;
                auto r = pos;
                auto b = brightness - r;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
            else if (pos < section2)
            {
                auto g = pos - section;
                auto r = brightness - g;
                auto b = 0;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
            else if (pos)
            {
                auto b = pos - section2;
                auto g = brightness - b;
                auto r = 0;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
        }
        m_pos++;
    }
    else if (m_mapping.pattern == PatternFade)
    {
        m_device->set_val(m_pos);
        if (m_dir)
        {
            m_pos -= 127;
        }
        else
        {
            m_pos += 127;
        }
        if (m_pos >= UINT16_MAX && !m_dir)
        {
            m_dir = true;
            m_pos -= 127;
        }
        if (m_pos == 0 && m_dir)
        {
            m_dir = false;
        }
    }
    m_next_poll = millis() + m_speed;
}
void StaticLedMapping::update(bool full_poll, bool send_events)
{
    m_device->set_val(UINT16_MAX);
}
void RgbLedDevice::set_val(uint16_t val)
{
    uint16_t r = val * scaleR;
    uint16_t g = val * scaleG;
    uint16_t b = val * scaleB;
    uint16_t w = val * scaleBrightness;
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], r, g, b, w);
    }
}
void RgbLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b, brightness);
}
uint8_t RgbLedDevice::led_count()
{
    return m_device.activeLed_count;
}
bool RgbLedDevice::supports_brightness()
{
    return m_led_device->supports_brightness();
}
void STP16CPCLedDevice::set_val(uint16_t val)
{
    for (int i = 0; i < m_device.activeLed_count; i++)
    {
        m_led_device->set_led(m_device.activeLed[i], val, val, val, 0);
    }
}
void STP16CPCLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    m_led_device->set_led(m_device.activeLed[i], r, g, b, brightness);
}
uint8_t STP16CPCLedDevice::led_count()
{
    return m_device.activeLed_count;
}
bool STP16CPCLedDevice::supports_brightness()
{
    return false;
}
void GpioLedDevice::set_val(uint16_t val)
{
    pwm_set_gpio_level(m_device.pin, val);
}
void GpioLedDevice::set_val_raw(uint8_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    pwm_set_gpio_level(m_device.pin, r);
}
uint8_t GpioLedDevice::led_count()
{
    return 1;
}
bool GpioLedDevice::supports_brightness()
{
    return false;
}
void RgbLedDevice::setup()
{
    if (m_led_device->supports_brightness())
    {

        float startR = m_device.startR;
        float startG = m_device.startG;
        float startB = m_device.startB;
        float startW = m_device.startW;
        float endR = m_device.endR;
        float endG = m_device.endG;
        float endB = m_device.endB;
        float endW = m_device.endW;
        scaleR = (endR - startR) / UINT16_MAX + startR;
        scaleG = (endG - startG) / UINT16_MAX + startG;
        scaleB = (endB - startB) / UINT16_MAX + startB;
        scaleBrightness = (endW - startW) / UINT16_MAX + startW;
    }
    else
    {
        float startR = m_device.startR - (255 - m_device.startW);
        float startG = m_device.startG - (255 - m_device.startW);
        float startB = m_device.startB - (255 - m_device.startW);
        float endR = m_device.endR - (255 - m_device.endW);
        float endG = m_device.endG - (255 - m_device.endW);
        float endB = m_device.endB - (255 - m_device.endW);
        scaleR = (endR - startR) / UINT16_MAX + startR;
        scaleG = (endG - startG) / UINT16_MAX + startG;
        scaleB = (endB - startB) / UINT16_MAX + startB;
        scaleBrightness = 0;
    }
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