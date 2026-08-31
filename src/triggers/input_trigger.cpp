#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"

InputActivationTrigger::InputActivationTrigger(bool any_time, proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_activation_trigger(activation_trigger), m_input(std::move(input)), m_any_time(any_time)
{
}

bool InputActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto val = m_input->tickDigital();

    if (m_activation_trigger.inverted)
    {
        val = !val;
    }
    if (m_activation_trigger.has_trigger)
    {
        auto analog_val = m_input->tickAnalog();
        if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_JoyHigh)
        {
            val = m_input->tickAnalog() > m_activation_trigger.triggerValue;
        }
        else if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_JoyLow)
        {
            val = m_input->tickAnalog() < m_activation_trigger.triggerValue;
        }
        else if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_Exact)
        {
            val = m_input->tickAnalog() == m_activation_trigger.triggerValue;
        }
        else if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_Range)
        {
            val = m_input->tickAnalog() > m_activation_trigger.triggerValue && m_input->tickAnalog() < m_activation_trigger.maxTriggerValue;
        }
        if (m_activation_trigger.inverted)
        {
            val = !val;
        }
        if (send_events && (analog_val != m_last_analog_val || full_poll))
        {
            m_last_analog_val = analog_val;
            proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, val}}};
            HIDConfigDevice::send_event(event, false);
        }
    }
    else if (send_events && (val != m_last_val || full_poll))
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, val}}};
        HIDConfigDevice::send_event(event, false);
    }
    if (!m_initialised)
    {
        m_initialised = true;
        m_last_val = val;
    }
    if (((m_any_time || mode_recently_changed()) && val != m_last_val))
    {
        reload();
    }
    m_last_val = val;
    return val;
}
