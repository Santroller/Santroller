#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
#include "emulation/usb/hid_device.h"
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

void ButtonMapping::update(bool full_poll, bool send_events)
{
    uint16_t event_value;
    bool event_driven = m_input->consumes_events();
    bool event_received = event_driven && m_input->consume_event(event_value);
    auto calcVal = event_driven ? event_received : m_input->tick_digital();

    if (!event_driven && m_mapping.inverted) {
        calcVal = !calcVal;
    }
    if (m_mapping.has_trigger)
    {
        auto val = event_driven ? (event_received ? event_value : 0) : m_input->tick_analog();
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
        if (send_events && (val != m_last_sent_value || full_poll))
        {
            proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, val, calcVal ? (uint16_t)65535 : (uint16_t)0}}};
            HIDConfigDevice::send_event(event, false);
            m_last_sent_value = val;
        }
    }
    else if (send_events && (calcVal != m_last_sent_value || full_poll))
    {
        proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, calcVal, calcVal}}};
        HIDConfigDevice::send_event(event, false);
        m_last_sent_value = calcVal;
    }
    if (calcVal)
    {
        m_last_poll = millis();
        m_last_value = calcVal;
    }
    else if (!m_mapping.has_debounce || (millis() - m_last_poll) > m_mapping.debounce)
    {
        m_last_value = calcVal;
    }
}
void AxisMapping::update(bool full_poll, bool send_events)
{
    uint16_t event_value;
    bool event_driven = m_input->consumes_events();
    bool event_received = event_driven && m_input->consume_event(event_value);
    auto uncalibrated = event_driven ? (event_received ? event_value : 0) : m_input->tick_analog();
    auto val = uncalibrated;
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
    else
    {
        val = calibrate(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, m_trigger);
    }
    if (val != m_mapping.center)
    {
        m_last_poll = millis();
        if ((!m_mapping.has_peakBased && !m_mapping.peakBased) || val > m_calibrated_value)
        {
            m_calibrated_value = val;
        }
    }
    else if (!m_mapping.has_debounce || (millis() - m_last_poll) > m_mapping.debounce)
    {
        m_calibrated_value = val;
    }
    m_centered = m_calibrated_value == (uint32_t)m_mapping.center;

    if (send_events && (uncalibrated != m_last_sent_value || m_calibrated_value != m_last_sent_calibrated_value || full_poll))
    {
        m_last_sent_value = uncalibrated;
        m_last_sent_calibrated_value = m_calibrated_value;
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, uncalibrated, m_calibrated_value}}};
        HIDConfigDevice::send_event(event, false);
    }
}