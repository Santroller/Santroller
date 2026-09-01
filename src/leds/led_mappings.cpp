#include "leds/leds.hpp"
#include "instance.hpp"
#include "utils.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include "emulation/usb/hid_device.h"
void InputLedMapping::update(bool full_poll, bool send_events)
{
    uint16_t raw = m_input->tickAnalog();
    uint16_t curr = (raw - m_mapping.min) * m_multiplier;
    if (send_events && ((full_poll || (raw != m_last_val)) && (millis() - m_last_poll) > 10))
    {
        m_last_val = raw;
        proto_Event event = {which_event : proto_Event_led_tag, event : {led : {m_id, raw, curr}}};
        HIDConfigDevice::send_event(event, false);
        m_last_poll = millis();
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
InputLedMapping::InputLedMapping(std::unique_ptr<LedMappingDevice> device, proto_InputLedMapping mapping, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_input(std::move(input)), m_mapping(mapping)
{
    m_multiplier = (UINT16_MAX) / (m_mapping.max - m_mapping.min);
}
PatternLedMapping::PatternLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PatternLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping), m_speed(mapping.speed ? mapping.speed : 1), m_brightness(mapping.brightness ? mapping.brightness : 1)
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
