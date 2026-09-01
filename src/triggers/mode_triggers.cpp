#include "triggers/mode_triggers.hpp"
#include "managers/config_manager.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"

UsbModeActivationTrigger::UsbModeActivationTrigger(proto_UsbDeviceAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_config(config)
{
}

bool UsbModeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (!m_config.has_consoleType)
    {
        if (send_events && full_poll)
        {
            proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
            HIDConfigDevice::send_event(event, true);
        }
        return true;
    }
    bool matched = false;
    switch (m_profile->mode)
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

bool UsbModeActivationTrigger::forcedConsoleMode(ConsoleMode& mode) const
{
    if (!m_config.has_forcedType)
    {
        return false;
    }
    mode = m_config.forcedType;
    return true;
}

BluetoothModeActivationTrigger::BluetoothModeActivationTrigger(proto_BluetoothMode mode, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_mode(mode)
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

WiiExtensionEmulationActivationTrigger::WiiExtensionEmulationActivationTrigger(proto_WiimoteAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_config(config)
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

PS2ControllerEmulationActivationTrigger::PS2ControllerEmulationActivationTrigger(proto_PSXAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_config(config)
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
