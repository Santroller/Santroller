#include "leds/base_led.hpp"
#include "utils.h"
#include <algorithm>

// How long one cycle of each effect takes at speed 20, scaled up by 21 - speed so speed 1 is 20 times
// slower. Breathing and rainbow match the timing of the old Pattern Based mode.
static constexpr uint32_t BREATHING_PERIOD_MS = 1032;
static constexpr uint32_t BLINK_PERIOD_MS = 100;
static constexpr uint32_t RAINBOW_PERIOD_MS = 256;

void LedMappingDevice::set_effect(LedEffectType type, uint32_t speed)
{
    m_effect_type = type;
    m_effect_speed = std::clamp<uint32_t>(speed, 1, 20);
    m_written_leds.clear();
}

void LedMappingDevice::forget_written()
{
    m_written_leds.clear();
}

LedMappingDevice::Written *LedMappingDevice::written_at(uint8_t index)
{
    if (m_written_leds.empty())
    {
        m_written_leds.resize(led_count());
    }
    return index < m_written_leds.size() ? &m_written_leds[index] : nullptr;
}

void LedMappingDevice::set_val(uint16_t val)
{
    track(val);
    m_lit |= val != 0;
    if (m_effect_type)
    {
        uint8_t count = led_count();
        for (uint8_t i = 0; i < count; i++)
        {
            if (Written *w = written_at(i))
            {
                *w = {val, 0, 0, 0, 0, false, true};
                draw(i, false);
            }
        }
        return;
    }
    if (!InactivityManager::instance().leds_off())
    {
        write_val(val);
    }
}

void LedMappingDevice::set_val_at(uint8_t index, uint16_t val)
{
    track(index);
    track(val);
    m_lit |= val != 0;
    if (m_effect_type)
    {
        if (Written *w = written_at(index))
        {
            *w = {val, 0, 0, 0, 0, false, true};
            draw(index, false);
        }
        return;
    }
    if (!InactivityManager::instance().leds_off())
    {
        write_val_at(index, val);
    }
}

void LedMappingDevice::set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    track(index);
    track(r);
    track(g);
    track(b);
    track(brightness);
    m_lit |= r || g || b;
    if (m_effect_type)
    {
        if (Written *w = written_at(index))
        {
            *w = {0, r, g, b, brightness, true, true};
            draw(index, false);
        }
        return;
    }
    if (!InactivityManager::instance().leds_off())
    {
        write_val_raw(index, r, g, b, brightness);
    }
}

void LedMappingDevice::redraw_effect()
{
    for (uint8_t i = 0; i < m_written_leds.size(); i++)
    {
        draw(i, true);
    }
}

// The rainbow's colour at pos (0 - 255), at full brightness
static void wheel(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b)
{
    if (pos < 85)
    {
        r = pos * 3;
        g = 0;
        b = 255 - r;
    }
    else if (pos < 170)
    {
        g = (pos - 85) * 3;
        r = 255 - g;
        b = 0;
    }
    else
    {
        b = (pos - 170) * 3;
        g = 255 - b;
        r = 0;
    }
}

void LedMappingDevice::draw(uint8_t index, bool redraw)
{
    const Written &w = m_written_leds[index];
    if (!w.set || InactivityManager::instance().leds_off())
    {
        return;
    }
    bool rgb = is_rgb();
    uint8_t r = w.r, g = w.g, b = w.b, bright = w.brightness;
    bool lit = w.raw ? (r || g || b) : w.val != 0;
    if (!lit)
    {
        // A released colour shows as it is, and it was drawn when it was written
        if (redraw)
        {
            return;
        }
        if (w.raw)
        {
            write_val_raw(index, r, g, b, bright);
        }
        else
        {
            write_val_at(index, w.val);
        }
        return;
    }
    uint32_t now = millis();
    uint32_t steps = 21 - m_effect_speed;
    if (m_effect_type == EffectRainbow)
    {
        if (!w.raw && !rgb)
        {
            // Nothing to colour
            if (!redraw)
            {
                write_val_at(index, w.val);
            }
            return;
        }
        if (!w.raw && !colour(w.val, r, g, b, bright))
        {
            return;
        }
        // At the brightness the mapping shows, each LED a step further around the rainbow than the last
        uint8_t amount = std::max({r, g, b});
        uint32_t period = RAINBOW_PERIOD_MS * steps;
        uint8_t count = m_written_leds.size();
        uint8_t pos = ((now % period) * 256 / period + index * 256 / count) & 0xFF;
        wheel(pos, r, g, b);
        write_val_raw(index, r * amount / 255, g * amount / 255, b * amount / 255, bright);
        return;
    }
    // How much of what the mapping shows comes through, 0 - 255
    uint32_t level;
    if (m_effect_type == EffectBreathing)
    {
        // Down to off and back up
        uint32_t period = BREATHING_PERIOD_MS * steps;
        uint32_t phase = now % period;
        level = phase < period / 2 ? 255 - phase * 510 / period : (phase - period / 2) * 510 / period;
        level = std::min<uint32_t>(level, 255);
    }
    else if (m_effect_type == EffectBlink)
    {
        uint32_t period = BLINK_PERIOD_MS * steps;
        level = now % period < period / 2 ? 255 : 0;
    }
    else
    {
        level = 255;
    }
    if (w.raw)
    {
        write_val_raw(index, r * level / 255, g * level / 255, b * level / 255, bright);
        return;
    }
    uint16_t val = (uint32_t)w.val * level / 255;
    if (rgb)
    {
        // Fades towards the released colour, or to off without one. The mapping lights the LED, so off is
        // drawn rather than left to whatever else drives it.
        if (!colour(val, r, g, b, bright))
        {
            r = g = b = 0;
        }
        write_val_raw(index, r, g, b, bright);
        return;
    }
    write_val_at(index, val);
}
