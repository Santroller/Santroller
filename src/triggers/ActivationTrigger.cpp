#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "usb/device/hid_device.h"

bool ActivationTriggerList::validate(bool claim_devices, bool full_poll, bool send_events)
{
    if (claim_devices && m_claimed)
    {
        return true;
    }
    if (triggers.empty())
    {
        return false;
    }
    for (auto &trigger : triggers)
    {
        if (!trigger->validate(false, full_poll, send_events))
        {
            return false;
        }
    }
    if (!claim_devices)
    {
        return true;
    }
    for (auto &trigger : triggers)
    {
        if (!trigger->validate(true, full_poll, send_events))
        {
            printf("trigger was not valid, this shouldnt happen!\r\n");
            return false;
        }
    }
    m_claimed = true;
    return true;
}
int ActivationTriggerList::assignedDevices()
{
    int assigned = 0;
    for (auto &trigger : triggers)
    {
        assigned |= trigger->assignedDevices();
    }
    return assigned;
}
InputActivationTrigger::InputActivationTrigger(bool any_time, proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_activation_trigger(activation_trigger), m_input(std::move(input)), m_any_time(any_time)
{
}

// deal with debounce and all the other fun things
bool InputActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto val = m_input->tickDigital();
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
    if (((m_any_time || mode_recently_changed()) && val != m_last_val) && HIDConfigDevice::tool_closed())
    {
        reload();
    }
    m_last_val = val;
    return val;
}

UsbModeActivationTrigger::UsbModeActivationTrigger(proto_UsbDeviceAssignment config, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_config(config)
{
}

bool UsbModeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (!m_config.has_consoleType)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
        return true;
    }
    bool matched = false;
    auto profile = all_profiles[m_profile_id];
    switch (profile->mode)
    {
    case ModeGuitarHeroArcade:
    case ModeHid:
        matched = m_config.consoleType == ConsolePC;
        break;
    case ModeOgXbox:
        matched = m_config.consoleType == ConsoleOgXbox;
        break;
    case ModeXbox360:
        matched = m_config.consoleType == ConsoleXbox360;
        break;
    case ModeXboxOne:
        matched = m_config.consoleType == ConsoleXboxOne;
        break;
    case ModePs3:
        matched = m_config.consoleType == ConsolePS3;
        break;
    case ModePs4:
        matched = m_config.consoleType == ConsolePS4_PS5;
        break;
    case ModePs5:
        matched = m_config.consoleType == ConsolePS4_PS5;
        break;
    case ModeWiiRb:
        matched = m_config.consoleType == ConsoleWii_WiiU;
        break;
    case ModeSwitch:
        matched = m_config.consoleType == ConsoleSwitch_Switch2;
        break;
    default:
        break;
    }

    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, matched}}};
        HIDConfigDevice::send_event(event, true);
    }
    return matched;
}

WiiExtTypeActivationTrigger::WiiExtTypeActivationTrigger(proto_WiiExtType type, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_type(type)
{
}

bool WiiExtTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto device = *it;
        if (device->is_wii_extension(m_type))
        {
            if (claim_device)
            {
                assignable_devices.erase(it);
                all_profiles[m_profile_id]->devices[device->m_id] = device;
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

PS2ControllerTypeActivationTrigger::PS2ControllerTypeActivationTrigger(proto_PS2ControllerType type, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_type(type)
{
}

bool PS2ControllerTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto device = *it;
        if (device->is_ps2_device(m_type))
        {
            if (claim_device)
            {
                assignable_devices.erase(it);
                all_profiles[m_profile_id]->devices[device->m_id] = device;
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

UsbTypeActivationTrigger::UsbTypeActivationTrigger(proto_SubType type, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_type(type)
{
}

bool UsbTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto &device = *it;
        if (device->is_usb_type(m_type))
        {
            if (claim_device)
            {
                auto &profile = all_profiles[m_profile_id];
                profile->devices.insert_or_assign(device->m_id, device);
                assignable_devices.erase(it);
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

SpecificUsbDeviceActivationTrigger::SpecificUsbDeviceActivationTrigger(proto_SpecificUsbDevice device, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_device(device)
{
}

bool SpecificUsbDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto device = *it;
        if (device->is_usb_device(m_device))
        {
            if (claim_device)
            {
                assignable_devices.erase(it);
                all_profiles[m_profile_id]->devices[device->m_id] = device;
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

BluetoothTypeActivationTrigger::BluetoothTypeActivationTrigger(proto_SubType type, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_type(type)
{
}

bool BluetoothTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto device = *it;
        if (device->is_bluetooth_type(m_type))
        {
            if (claim_device)
            {
                assignable_devices.erase(it);
                all_profiles[m_profile_id]->devices[device->m_id] = device;
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

SpecificBluetoothDeviceActivationTrigger::SpecificBluetoothDeviceActivationTrigger(proto_SpecificUsbDevice device, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_device(device)
{
}

bool SpecificBluetoothDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto device = *it;
        if (device->is_bluetooth_device(m_device))
        {
            if (claim_device)
            {
                assignable_devices.erase(it);
                all_profiles[m_profile_id]->devices[device->m_id] = device;
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

MidiChannelActivationTrigger::MidiChannelActivationTrigger(uint32_t channel, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_channel(channel)
{
}

bool MidiChannelActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto it = assignable_devices.begin();
    while (it != assignable_devices.end())
    {
        auto device = *it;
        if (device->has_midi_channel(m_channel - 1))
        {
            if (claim_device)
            {
                assignable_devices.erase(it);
                printf("Claimed device: %d %p %p\r\n", m_profile_id, all_profiles[m_profile_id], device);
                all_profiles[m_profile_id]->midiDevices[device->m_id] = device;
            }
            m_last_val = true;
            return true;
        }
        it++;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

CatchAllActivationTrigger::CatchAllActivationTrigger(proto_CatchallAssignment config, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_config(config)
{
}

bool CatchAllActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}
BluetoothModeActivationTrigger::BluetoothModeActivationTrigger(proto_BluetoothMode mode, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_mode(mode)
{
}

bool BluetoothModeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}
WiiExtensionEmulationActivationTrigger::WiiExtensionEmulationActivationTrigger(proto_WiimoteAssignment config, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_config(config)
{
}

bool WiiExtensionEmulationActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}
PS2ControllerEmulationActivationTrigger::PS2ControllerEmulationActivationTrigger(proto_PSXAssignment config, uint32_t profile_id, uint32_t id, uint32_t list_id) : ActivationTrigger(profile_id, id, list_id), m_config(config)
{
}

bool PS2ControllerEmulationActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}