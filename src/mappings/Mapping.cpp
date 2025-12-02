#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>
#include <stdint.h>
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

void ButtonMapping::update(bool full_poll)
{
    auto calcVal = m_input->tickDigital();
    if (m_mapping.has_trigger)
    {
        auto val = m_input->tickAnalog();
        calcVal = false;
        if (m_mapping.trigger == AnalogToDigitalTriggerType_JoyHigh)
        {
            calcVal = m_input->tickAnalog() > m_mapping.triggerValue;
        }
        else if (m_mapping.trigger == AnalogToDigitalTriggerType_JoyLow)
        {
            calcVal = m_input->tickAnalog() < m_mapping.triggerValue;
        }
        else if (m_mapping.trigger == AnalogToDigitalTriggerType_Exact)
        {
            calcVal = m_input->tickAnalog() == m_mapping.triggerValue;
        }
        if (val != m_lastValueTrigger || full_poll)
        {
            proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, val, calcVal ? (uint16_t)65535 : (uint16_t)0}}};
            send_event(event);
            m_lastValueTrigger = val;
        }
    }
    else
    {
        if (calcVal != m_lastValue || full_poll)
        {
            proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, calcVal, calcVal}}};
            send_event(event);
        }
    }
    if (calcVal)
    {
        m_lastPoll = micros();
        m_lastValue = calcVal;
    }
    else if (!m_mapping.has_debounce || (micros() - m_lastPoll) > m_mapping.debounce)
    {
        m_lastValue = calcVal;
    }
}
void AxisMapping::update(bool full_poll)
{
    auto val = m_input->tickAnalog();
    if (val != m_lastValue || full_poll)
    {
        m_lastValue = val;
        m_calibratedValue = calibrate(val, m_mapping.max, m_mapping.min, m_mapping.deadzone, m_mapping.center, m_trigger);
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, m_lastValue, m_calibratedValue}}};
        send_event(event);
    }
    // TODO: better solution for this probably.
    m_centered = !m_centered;
}