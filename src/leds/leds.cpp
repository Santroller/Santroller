#include "leds/leds.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "usb/device/hid_device.h"
void InputLedMapping::update(bool full_poll, bool send_events)
{
    uint16_t raw = m_input->tickAnalog();
    uint16_t curr = (raw - m_mapping.min) * m_multiplier;
    if (send_events && ((full_poll || (raw != m_last_val || m_resend)) && (millis() - m_last_poll) > 10))
    {
        m_last_val = raw;
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, raw, curr}}};
        m_resend = !HIDConfigDevice::send_event(event);
        if (!m_resend)
        {
            m_last_poll = millis();
        }
    }
    if (m_mapping.has_pattern && m_mapping.pattern == PatternHeatmap)
    {
        if (curr)
        {
            if (millis() - m_last_increase > 10)
            {
                m_pos += 2048;
                if (m_pos > UINT16_MAX)
                {
                    m_pos = UINT16_MAX;
                }
            }
            m_last_increase = millis();
        }
        if (!curr && millis() - m_last_decay > 200)
        {
            if (m_pos <= 2048)
            {
                m_pos = 0;
            }
            else
            {
                m_pos -= 1024;
            }
            m_last_decay = millis();
        }
        m_device->set_val(m_pos);
    }
    else
    {
        m_device->set_val(curr);
    }
}
InputLedMapping::InputLedMapping(std::unique_ptr<LedMappingDevice> device, proto_InputLedMapping mapping, std::unique_ptr<Input> input, uint32_t profile_id, uint32_t id) : LedMapping(std::move(device), profile_id, id), m_input(std::move(input)), m_mapping(mapping)
{
    m_multiplier = (UINT16_MAX) / (m_mapping.max - m_mapping.min);
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
    uint32_t speed = m_speed;
    uint8_t leds = m_device->led_count();
    // If the led doesn't support brightness, then we just stop each channel at a lower brightness
    uint8_t pos_per_chan = m_device->supports_brightness() ? 255 : m_brightness;
    if (m_mapping.pattern == PatternRainbow)
    {
        uint8_t section = pos_per_chan / 3;
        uint8_t section2 = section * 2;
        for (int i = 0; i < leds; i++)
        {
            auto pos = (i * pos_per_chan / leds + m_pos) % pos_per_chan;
            if (pos < section)
            {
                auto g = 0;
                auto r = pos;
                auto b = pos_per_chan - r;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
            else if (pos < section2)
            {
                auto g = pos - section;
                auto r = pos_per_chan - g;
                auto b = 0;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
            else if (pos)
            {
                auto b = pos - section2;
                auto g = pos_per_chan - b;
                auto r = 0;
                m_device->set_val_raw(i, r, g, b, m_brightness);
            }
        }
        m_pos++;

        if (!m_device->supports_brightness())
        {
            // Speed needs to be scaled in this scenario, as there are less values being looped over
            speed *= 255 / m_brightness;
        }
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
    m_next_poll = millis() + speed;
}
void StaticLedMapping::update(bool full_poll, bool send_events)
{
    m_device->set_val(UINT16_MAX);
}
void RgbLedDevice::set_val(uint16_t val)
{
    uint16_t r = ((float)val * scaleR) + startR;
    uint16_t g = ((float)val * scaleG) + startG;
    uint16_t b = ((float)val * scaleB) + startB;
    uint16_t w = ((float)val * scaleBrightness) + m_device.startW;
    if (!m_device.hasStart && !r && !g && !b)
    {
        return;
    }
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
    startR = m_device.startR;
    startG = m_device.startG;
    startB = m_device.startB;
    if (!m_device.hasStart)
    {
        startR = startG = startB = 0;
    }
    endR = m_device.endR;
    endG = m_device.endG;
    endB = m_device.endB;
    if (m_led_device->supports_brightness())
    {
        float startW = m_device.startW;
        float endW = m_device.endW;
        scaleBrightness = ((float)endW - startW) / UINT16_MAX;
    }
    else
    {

        startR *= m_device.startW / 255.0f;
        startG *= m_device.startW / 255.0f;
        startB *= m_device.startW / 255.0f;
        endR *= m_device.endW / 255.0f;
        endG *= m_device.endW / 255.0f;
        endB *= m_device.endW / 255.0f;
        scaleBrightness = 0;
    }
    scaleR = ((float)endR - startR) / UINT16_MAX;
    scaleG = ((float)endG - startG) / UINT16_MAX;
    scaleB = ((float)endB - startB) / UINT16_MAX;
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
