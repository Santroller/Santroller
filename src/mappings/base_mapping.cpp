#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
#include "emulation/usb/hid_device.h"
#include "input/shortcut.hpp"
#include "managers/inactivity_manager.hpp"
#include "managers/config_manager.hpp"
#include "pico/bootrom.h"

uint16_t Mapping::sample_ui_event()
{
    uint16_t value = 0;
    if (m_input->peek_event(value))
    {
        m_ui_event_value = value;
        m_ui_event_time = millis();
    }
    const uint32_t hold = (m_mapping.has_debounce && m_mapping.debounce_us >= 1000) ? m_mapping.debounce_us / 1000 : 100;
    return (millis() - m_ui_event_time <= hold) ? m_ui_event_value : 0;
}

uint16_t Mapping::calibrate(float val, float max, float min, float deadzone, float center, bool trigger)
{
    if (trigger)
    {
        auto inverted = min > max;
        if (inverted)
        {
            min -= deadzone;
            if (val > min)
                return 0;
            if (val < max)
                val = max;
        }
        else
        {
            min += deadzone;
            if (val < min)
                return 0;
            if (val > max)
                val = max;
        }
        val = map(val, min, max, 0, UINT16_MAX);
    }
    else
    {

        auto inverted = min > max;
        if (inverted)
        {
            if (val < center)
            {
                if (center - val < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, center - deadzone, max, UINT16_MAX / 2, UINT16_MAX);
            }
            else
            {
                if (val - center < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, min, center + deadzone, 0, UINT16_MAX / 2);
            }
        }
        else
        {
            if (val < center)
            {
                if (center - val < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, min, center - deadzone, 0, UINT16_MAX / 2);
            }
            else
            {
                if (val - center < deadzone)
                {
                    return UINT16_MAX / 2;
                }

                val = map(val, center + deadzone, max, UINT16_MAX / 2, UINT16_MAX);
            }
        }
    }
    if (val > UINT16_MAX)
        val = UINT16_MAX;
    if (val < 0)
        val = 0;
    return val;
}

void ButtonMapping::sample(bool full_poll, bool send_events)
{
    uint16_t event_value = 0;
    uint16_t trigger_value = 0;
    bool event_driven = m_input->consumes_events();
    if (event_driven && !send_events)
    {
        m_input->consume_event(event_value);
    }
    if (event_driven && send_events)
    {
        event_value = sample_ui_event();
    }
    auto calcVal = event_driven ? event_value > 0 : m_input->tick_digital();
    if (!event_driven && m_mapping.inverted) {
        calcVal = !calcVal;
    }
    if (m_mapping.has_trigger)
    {
        auto val = event_driven ? event_value : m_input->tick_analog();
        trigger_value = val;
        calcVal = false;
        if (m_mapping.trigger == AnalogToDigitalTriggerType_JoyHigh)
        {
            calcVal = val > m_mapping.triggerValue;
        }
        else if (m_mapping.trigger == AnalogToDigitalTriggerType_JoyLow)
        {
            calcVal = val < m_mapping.triggerValue;
        }
        else if (m_mapping.trigger == AnalogToDigitalTriggerType_Exact)
        {
            calcVal = val == m_mapping.triggerValue;
        }
        else if (m_mapping.trigger == AnalogToDigitalTriggerType_Range)
        {
            calcVal = val > m_mapping.triggerValue && val < m_mapping.maxTriggerValue;
        }
        if (!event_driven && m_mapping.inverted) {
            calcVal = !calcVal;
        }
    }

    bool physical_pressed = calcVal;

    // Release latching: if previously suppressed by a shortcut, remain suppressed
    // until the physical input is released.
    if (m_waiting_for_release)
    {
        if (!physical_pressed)
        {
            m_waiting_for_release = false;
        }
    }

    // If this mapping is a shortcut that masks other mappings:
    if (!m_masked_mappings.empty())
    {
        auto *shortcut = m_input ? m_input->as_shortcut() : nullptr;
        bool chord_active = shortcut ? shortcut->tick_digital() : physical_pressed;
        if (chord_active)
        {
            for (auto *masked : m_masked_mappings)
            {
                masked->mask_by_shortcut();
            }
        }
    }

    uint16_t pressure = m_mapping.has_trigger ? trigger_value
        : event_driven ? event_value
        : m_input->has_independent_analog_value() ? m_input->tick_analog()
        : calcVal ? UINT16_MAX : 0;

    if (m_suppressed || m_waiting_for_release)
    {
        calcVal = false;
        m_live_value = false;
        m_live_pressure = 0;
    }

    m_suppressed = false;

    if (send_events)
    {
        if (m_mapping.has_trigger)
        {
            if (pressure != m_last_sent_pressure || calcVal != m_last_sent_value || full_poll)
            {
                proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, pressure, calcVal ? (uint32_t)65535 : (uint32_t)0}}};
                HIDConfigDevice::send_event(event, false);
                m_last_sent_pressure = pressure;
                m_last_sent_value = calcVal;
            }
        }
        else
        {
            if (calcVal != m_last_sent_value || full_poll)
            {
                proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, calcVal, calcVal}}};
                HIDConfigDevice::send_event(event, false);
                m_last_sent_value = calcVal;
            }
            if (m_input->has_independent_analog_value() && (pressure != m_last_sent_pressure || full_poll))
            {
                proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, pressure, calcVal ? (uint32_t)65535 : (uint32_t)0}}};
                HIDConfigDevice::send_event(event, false);
                m_last_sent_pressure = pressure;
            }
        }
    }
    // With combined strum debounce, a new strum can't start while the other direction is
    // still held or debouncing, so a bouncing strum switch can't register both ways
    if (calcVal && !m_live_value && m_strum_bit >= 0 && m_profile->combined_strum_debounce &&
        m_profile->other_strum_live(m_strum_bit))
    {
        calcVal = false;
    }
    bool was_live = m_live_value;
    if (calcVal)
    {
        m_last_poll = time_us_64();
        m_live_value = calcVal;
        m_live_pressure = m_mapping.inverted && !m_mapping.has_trigger ? UINT16_MAX : pressure;
    }
    else if (!m_mapping.has_debounce || (time_us_64() - m_last_poll) > m_mapping.debounce_us)
    {
        m_live_value = calcVal;
        m_live_pressure = 0;
    }
    // Changes rather than held buttons count, so an input stuck on (like a toggle) can't keep the controller awake
    if (m_live_value != was_live)
    {
        InactivityManager::instance().input_activity(millis());
    }
}
void ActionMapping::update_action()
{
    sample(false, false);
    bool pressed = live_value();
    bool was_pressed = m_was_pressed;
    m_was_pressed = pressed;
    if (!pressed || was_pressed)
    {
        return;
    }
    switch (m_mapping.mapping.mapping.action)
    {
    case ActionRestartDeviceStack:
        ConfigManager::instance().request_device_stack_restart();
        break;
    case ActionBootloader:
        reset_usb_boot(0, 0);
        break;
    }
}

void ButtonMapping::update(bool full_poll, bool send_events)
{
    sample(full_poll, send_events);
    if (m_queue_bit >= 0)
    {
        bool pressed = m_profile->input_queue.presented(m_queue_bit);
        m_last_value = pressed;
        m_last_pressure = pressed ? (m_live_value ? m_live_pressure : UINT16_MAX) : 0;
        return;
    }
    m_last_value = m_live_value;
    m_last_pressure = m_live_pressure;
}
void AxisMapping::update(bool full_poll, bool send_events)
{
    uint16_t event_value = 0;
    bool event_driven = m_input->consumes_events();
    if (send_events && event_driven)
    {
        event_value = sample_ui_event();
        uint32_t val = event_value;
        if (m_mapping.has_pressed)
        {
            val = event_value ? m_mapping.pressed : (m_mapping.has_released ? m_mapping.released : m_mapping.center);
        }
        else
        {
            val = calibrate(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, m_trigger);
        }
        if (event_value != m_last_sent_value || val != m_last_sent_calibrated_value || full_poll)
        {
            m_last_sent_value = event_value;
            m_last_sent_calibrated_value = val;
            proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, event_value, val}}};
            HIDConfigDevice::send_event(event, false);
        }
        return;
    }

    bool event_received = event_driven && m_input->consume_event(event_value);
    uint32_t uncalibrated = event_driven ? (event_received ? event_value : 0) : m_input->tick_analog();
    uint32_t val = uncalibrated;
    if (m_mapping.has_pressed)
    {
        if (event_driven ? event_received : m_input->tick_digital())
        {
            val = m_mapping.pressed;
        }
        else if (m_mapping.has_released)
        {
            val = m_mapping.released;
        }
        else
        {
            val = m_mapping.center;
        }
    }
    else if (m_mapping.section != AxisSectionFull)
    {
        // calibrate like a trigger, then use that to push the axis from its centre to one end
        uint32_t amount = calibrate(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, true) / 2;
        val = m_mapping.section == AxisSectionPositive ? UINT16_MAX / 2 + amount : UINT16_MAX / 2 - amount;
    }
    else
    {
        val = calibrate(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, m_trigger);
    }
    // A half axis rests at the axis centre, so it can leave the axis to the other half's mapping
    const uint32_t rest = m_mapping.section != AxisSectionFull ? UINT16_MAX / 2 : (uint32_t)m_mapping.center;

    bool physical_pressed = (val != rest);
    if (m_waiting_for_release)
    {
        if (!physical_pressed)
        {
            m_waiting_for_release = false;
        }
    }

    if (m_suppressed || m_waiting_for_release)
    {
        val = rest;
        m_calibrated_value = rest;
    }
    m_suppressed = false;

    if (val != rest)
    {
        m_last_poll = time_us_64();
        if ((!m_mapping.has_peakBased && !m_mapping.peakBased) || val > m_calibrated_value)
        {
            m_calibrated_value = val;
        }
    }
    else if (!m_mapping.has_debounce || (time_us_64() - m_last_poll) > m_mapping.debounce_us)
    {
        m_calibrated_value = val;
    }
    m_centered = m_calibrated_value == rest || !m_input->valid();

    if (send_events && (uncalibrated != m_last_sent_value || m_calibrated_value != m_last_sent_calibrated_value || full_poll))
    {
        m_last_sent_value = uncalibrated;
        m_last_sent_calibrated_value = m_calibrated_value;
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, uncalibrated, m_calibrated_value}}};
        HIDConfigDevice::send_event(event, false);
    }
}