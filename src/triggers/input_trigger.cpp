#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include "managers/config_manager.hpp"
#include "profiles/profile.hpp"

InputActivationTrigger::InputActivationTrigger(bool any_time, proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_activation_trigger(activation_trigger), m_input(std::move(input)), m_any_time(any_time)
{
    m_last_val = calc_val();
    m_last_analog_val = m_input->tick_analog();
}
void InputActivationTrigger::reset()
{
    m_input->link_device(false);
}

bool InputActivationTrigger::calc_val()
{
    auto val = m_input->tick_digital();

    if (m_activation_trigger.has_trigger)
    {
        if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_JoyHigh)
        {
            val = m_input->tick_analog() > m_activation_trigger.triggerValue;
        }
        else if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_JoyLow)
        {
            val = m_input->tick_analog() < m_activation_trigger.triggerValue;
        }
        else if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_Exact)
        {
            val = m_input->tick_analog() == m_activation_trigger.triggerValue;
        }
        else if (m_activation_trigger.trigger == AnalogToDigitalTriggerType_Range)
        {
            val = m_input->tick_analog() > m_activation_trigger.triggerValue && m_input->tick_analog() < m_activation_trigger.maxTriggerValue;
        }
    }
    if (m_activation_trigger.inverted)
    {
        val = !val;
    }
    return val;
}

bool InputActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    m_input->link_device(claim_device);
    auto val = calc_val();
    if (m_activation_trigger.has_trigger)
    {
        auto analog_val = m_input->tick_analog();
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
    bool latched = false;
    if (!m_any_time)
    {
        auto &config_mgr = ConfigManager::instance();
        const uint64_t key = startup_key();
        latched = config_mgr.is_startup_trigger_latched(key);
        if (!latched && val && mode_recently_changed())
        {
            config_mgr.latch_startup_trigger(key);
            // Seen for the first time after the config loaded (eg the input's device was still
            // starting up), so load it again to pick this profile
            if (val != m_last_val)
            {
                reload();
            }
            latched = true;
        }
    }
    else if (val != m_last_val && val)
    {
        reload();
    }
    m_last_val = val;
    return latched || (m_any_time && val);
}

uint64_t InputActivationTrigger::startup_key() const
{
    return ((uint64_t)m_profile->profile_id << 32) | ((uint32_t)m_list_id << 16) | m_id;
}
