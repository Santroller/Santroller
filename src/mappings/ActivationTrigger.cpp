#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"

InputActivationTrigger::InputActivationTrigger(proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, uint32_t profile_id) : ActivationTrigger(profile_id), m_activation_trigger(activation_trigger), m_input(std::move(input))
{
}

// deal with debounce and all the other fun things
void InputActivationTrigger::update(bool tool_closed)
{
    auto val = m_input->tickDigital();
    if (m_activation_trigger.has_trigger)
    {
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
    }
    if (val)
    {
        if (tool_closed)
        {
            set_current_profile(m_profile_id);
        }
        // TODO: can send events so the triggers light up in the tool
    }
}