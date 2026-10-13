#pragma once
#include <stdint.h>
#include <math.h>
#include "input.pb.h"

// Filters for the raw value of an analog input, run before it is calibrated. Each one is off
// unless configured, and they run in this order: low pass, ResponsiveAnalogRead, high pass,
// zero point drift compensation. Cutoffs use the real time between samples, as the main loop
// doesn't run at a fixed rate.
class AnalogFilter
{
public:
    // ResponsiveAnalogRead is tuned per sample, so it steps at a fixed rate instead
    static constexpr uint64_t RESPONSIVE_STEP_US = 1000;
    // How long zero point drift compensation takes to follow the input, in seconds
    static constexpr float ZPD_TIME_CONSTANT = 1.0f;

    AnalogFilter(const proto_AnalogFilter &config, float rest)
        : m_low_pass_rc(rc(config.has_lowPassCutoff, config.lowPassCutoff)),
          m_high_pass_rc(rc(config.has_highPassCutoff, config.highPassCutoff)),
          m_responsive(config.has_responsive && config.responsive),
          // the library works on 10 bit values, so scale its settings up to 16 bit
          m_responsive_snap((config.has_responsiveSnap ? clamp(config.responsiveSnap, 0, 1) : 0.01f) / 64),
          m_responsive_threshold((config.has_responsiveActivityThreshold ? config.responsiveActivityThreshold : 4.0f) * 64),
          m_zpd_window(config.has_zpdWindow ? config.zpdWindow : 0),
          m_rest(rest)
    {
    }

    void set_rest(float rest)
    {
        m_rest = rest;
    }

    bool enabled() const
    {
        return m_low_pass_rc > 0 || m_high_pass_rc > 0 || m_responsive || m_zpd_window;
    }

    uint16_t update(uint16_t raw, uint64_t now_us)
    {
        if (!enabled())
        {
            return raw;
        }
        float x = raw;
        if (!m_started)
        {
            m_started = true;
            m_last_us = now_us;
            m_responsive_last_us = now_us;
            m_low_pass = x;
            m_smooth = x;
            m_high_pass_in = x;
            m_high_pass = 0;
            m_zpd_offset = 0;
            m_out = raw;
            return m_out;
        }
        float dt = (now_us - m_last_us) / 1000000.0f;
        if (dt <= 0)
        {
            return m_out;
        }
        m_last_us = now_us;
        if (m_low_pass_rc > 0)
        {
            m_low_pass += (x - m_low_pass) * (dt / (m_low_pass_rc + dt));
            x = m_low_pass;
        }
        if (m_responsive)
        {
            if (now_us - m_responsive_last_us >= RESPONSIVE_STEP_US)
            {
                m_responsive_last_us = now_us;
                responsive_step(x);
            }
            x = m_smooth;
        }
        if (m_high_pass_rc > 0)
        {
            m_high_pass = (m_high_pass_rc / (m_high_pass_rc + dt)) * (m_high_pass + x - m_high_pass_in);
            m_high_pass_in = x;
            x = m_rest + m_high_pass;
        }
        if (m_zpd_window)
        {
            // only follow the input while it is near rest, so real movement is never removed
            if (fabsf(x - m_zpd_offset - m_rest) <= m_zpd_window)
            {
                m_zpd_offset += (x - m_rest - m_zpd_offset) * (dt / (ZPD_TIME_CONSTANT + dt));
            }
            x -= m_zpd_offset;
        }
        m_out = clamp(x, 0, UINT16_MAX) + 0.5f;
        return m_out;
    }

private:
    static float clamp(float x, float min, float max)
    {
        return x < min ? min : x > max ? max : x;
    }
    // RC time constant for a cutoff in Hz, 0 when the filter is off
    static float rc(bool has, float cutoff)
    {
        return has && cutoff > 0 ? 1.0f / (2.0f * (float)M_PI * cutoff) : 0;
    }
    static float snap_curve(float x)
    {
        float y = 1.0f / (x + 1.0f);
        y = (1.0f - y) * 2.0f;
        return y > 1.0f ? 1.0f : y;
    }
    // ResponsiveAnalogRead::getResponsiveValue, with sleep and edge snap enabled
    void responsive_step(float value)
    {
        bool sleep = m_responsive_threshold > 0;
        // drag values near the ends towards them, so the ends can be reached without sleeping first
        if (sleep)
        {
            if (value < m_responsive_threshold)
            {
                value = value * 2 - m_responsive_threshold;
            }
            else if (value > UINT16_MAX - m_responsive_threshold)
            {
                value = value * 2 - UINT16_MAX + m_responsive_threshold;
            }
        }
        float diff = fabsf(value - m_smooth);
        m_error += ((value - m_smooth) - m_error) * 0.4f;
        if (sleep && fabsf(m_error) < m_responsive_threshold)
        {
            return;
        }
        float snap = snap_curve(diff * m_responsive_snap);
        m_smooth = clamp(m_smooth + (value - m_smooth) * snap, 0, UINT16_MAX);
    }

    float m_low_pass_rc = 0;
    float m_high_pass_rc = 0;
    bool m_responsive = false;
    float m_responsive_snap = 0;
    float m_responsive_threshold = 0;
    uint32_t m_zpd_window = 0;
    float m_rest = 0;

    bool m_started = false;
    uint64_t m_last_us = 0;
    uint64_t m_responsive_last_us = 0;
    float m_low_pass = 0;
    float m_smooth = 0;
    float m_error = 0;
    float m_high_pass_in = 0;
    float m_high_pass = 0;
    float m_zpd_offset = 0;
    uint16_t m_out = 0;
};

// Rapid trigger, based on minipad's KeyHandler::checkHEKey. Works on calibrated travel, where
// 0 is released and 65535 is fully pressed. Passing the actuation point presses and enters the
// rapid trigger zone. Inside it, moving back up by the up sensitivity from the deepest point
// releases, and moving down by the down sensitivity from the highest point presses again.
// The zone ends when the input goes back past the actuation point (by the up sensitivity, so
// noise there can't chatter), or with continuous mode only once it is nearly fully released.
class RapidTrigger
{
public:
    RapidTrigger(const proto_RapidTrigger &config, uint16_t actuation)
        : m_actuation(actuation),
          m_down(config.downSensitivity),
          m_up(config.upSensitivity),
          m_continuous(config.has_continuous && config.continuous)
    {
    }

    bool update(uint16_t travel)
    {
        int32_t t = travel;
        if (m_in_zone && (m_continuous ? t <= (int32_t)m_up : t + (int32_t)m_up <= m_actuation))
        {
            m_in_zone = false;
            m_pressed = false;
        }
        if (!m_in_zone && t >= m_actuation)
        {
            m_in_zone = true;
            m_pressed = true;
        }
        else if (m_in_zone)
        {
            if (!m_pressed && t >= m_peak + (int32_t)m_down)
            {
                m_pressed = true;
            }
            else if (m_pressed && t + (int32_t)m_up <= m_peak)
            {
                m_pressed = false;
            }
        }
        // while pressed this is the deepest point, while released the highest
        if (m_pressed ? t > m_peak : t < m_peak)
        {
            m_peak = t;
        }
        return m_pressed;
    }

private:
    int32_t m_actuation = 0;
    uint32_t m_down = 0;
    uint32_t m_up = 0;
    bool m_continuous = false;
    bool m_in_zone = false;
    bool m_pressed = false;
    int32_t m_peak = 0;
};
