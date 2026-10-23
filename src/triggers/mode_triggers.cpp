#include "triggers/mode_triggers.hpp"
#include "managers/config_manager.hpp"
#include "managers/device_manager.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/usb_host_presence.hpp"
#include "managers/profile_manager.hpp"
#include "profiles/profile.hpp"
#include "triggers/input_trigger.hpp"
#include "usb/auth_broker.h"

UsbModeActivationTrigger::UsbModeActivationTrigger(proto_UsbDeviceAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_config(config)
{
}

bool UsbModeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    bool eligible = true;
    bool matched = false;
    if (m_config.has_consoleType)
    {
        // A keyboard still belongs to its detected console after its output switches to HID.
        switch (ConfigManager::instance().get_detected_mode())
        {
        case ModeGuitarHeroArcade:
        case ModeSpice2x:
        case ModePdLoader:
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
        case ModeKeyboardMania:
            matched = m_config.consoleType == ConsolePS2;
            break;
        case ModeSwitch:
            matched = m_config.consoleType == ConsoleSwitch_Switch2;
            break;
        default:
            break;
        }
        eligible = matched;
    }
    // Without a host, a profile further down (eg bluetooth) gets the inputs instead
    if (!UsbHostPresence::instance().present())
    {
        eligible = false;
        matched = false;
    }

    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, matched}}};
        HIDConfigDevice::send_event(event, true);
    }
    else
    {
        m_last_val = matched;
    }
    return eligible;
}

bool UsbModeActivationTrigger::forcedConsoleMode(ConsoleMode& mode) const
{
    if (!m_config.has_forcedType)
    {
        return false;
    }

    switch (m_config.forcedType)
    {
    case ModeHid:
    case ModeOgXbox:
    case ModeXbox360:
    case ModeXboxOne:
    case ModePs3:
    case ModePs4:
    case ModePs5:
    case ModeWiiRb:
    case ModeSwitch:
    case ModeSpice2x:
    case ModePdLoader:
    case ModeGuitarHeroArcade:
    case ModeKeyboardMania:
        break;
    default:
        printf("Ignoring invalid forced console mode: %d\r\n", m_config.forcedType);
        return false;
    }

    mode = m_config.forcedType;
    return true;
}

bool UsbModeActivationTrigger::xinputOnWindows(bool &enabled) const
{
    if (!m_config.has_xinputOnWindows)
    {
        return false;
    }
    enabled = m_config.xinputOnWindows;
    return true;
}

bool UsbModeActivationTrigger::ps4OrPs5Mode(bool &enabled) const
{
    if (!m_config.has_ps4OrPs5Mode)
    {
        return false;
    }
    enabled = m_config.ps4OrPs5Mode;
    return true;
}

BluetoothModeActivationTrigger::BluetoothModeActivationTrigger(proto_BluetoothMode mode, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_mode(mode)
{
}

bool BluetoothModeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (send_events && (!m_last_val || full_poll))
    {
        m_last_val = true;
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
    if (!DeviceManager::instance().is_wii_communicating())
    {
        return false;
    }
    if (claim_device && ConfigManager::instance().has_seen_assignment(AssignWiimoteExtension))
    {
        return false;
    }
    if (send_events && (!m_last_val || full_poll))
    {
        m_last_val = true;
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
    if (!DeviceManager::instance().is_psx_communicating())
    {
        return false;
    }
    if (claim_device && ConfigManager::instance().has_seen_assignment(AssignPsx))
    {
        return false;
    }
    if (send_events && (!m_last_val || full_poll))
    {
        m_last_val = true;
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}

JoybusEmulationActivationTrigger::JoybusEmulationActivationTrigger(proto_JoybusAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_config(config)
{
}

bool JoybusEmulationActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (!DeviceManager::instance().is_joybus_communicating())
    {
        return false;
    }
    if (claim_device && ConfigManager::instance().has_seen_assignment(AssignJoybus))
    {
        return false;
    }
    if (send_events && (!m_last_val || full_poll))
    {
        m_last_val = true;
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}

SNESEmulationActivationTrigger::SNESEmulationActivationTrigger(proto_SNESAssignment config, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_config(config)
{
}

bool SNESEmulationActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    if (!DeviceManager::instance().is_snes_communicating())
    {
        return false;
    }
    if (claim_device && ConfigManager::instance().has_seen_assignment(AssignSnes))
    {
        return false;
    }
    if (send_events && (!m_last_val || full_poll))
    {
        m_last_val = true;
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, true}}};
        HIDConfigDevice::send_event(event, true);
    }
    return true;
}

XboxOneAuthActivationTrigger::XboxOneAuthActivationTrigger(proto_XboxOneAuthAssignment config, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id)
{
    if (input)
    {
        m_trigger = std::make_unique<InputActivationTrigger>(true, config.trigger, std::move(input), profile, id, list_id);
    }
}

XboxOneAuthActivationTrigger::~XboxOneAuthActivationTrigger() = default;

void XboxOneAuthActivationTrigger::reset()
{
    m_profile->temp_claimed_devices.erase({DeviceSlotKind::USB, m_slot_id});
    if (m_trigger)
    {
        m_trigger->reset();
    }
}

bool XboxOneAuthActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto &profile_mgr = ProfileManager::instance();
    // Anywhere else an Xbox One controller is just a controller
    bool xbox_one = ConfigManager::instance().get_requested_mode() == ModeXboxOne;
    std::shared_ptr<Device> pad;
    if (xbox_one)
    {
        auto is_pad = [](const auto &device)
        { return device && device->still_connected && device->is_xbox_one_pad(); };
        pad = m_profile->get_claimed_device(DeviceSlotKind::USB, m_slot_id);
        if (!is_pad(pad))
        {
            pad = DeviceManager::instance().find_assignable_device_if(is_pad);
        }
        if (pad)
        {
            // So the trigger can read it, eg a button held while plugging it in
            m_profile->temp_claimed_devices[{DeviceSlotKind::USB, m_slot_id}] = pad;
        }
    }
    // Always checked, so the config tool sees the input
    bool triggered = !m_trigger || m_trigger->validate(false, full_poll, send_events);
    bool matched = false;
    if (xbox_one && triggered && (pad || auth_broker.has_handler(ModeXboxOne)))
    {
        matched = true;
        if (claim_device)
        {
            if (pad && m_profile->get_claimed_device(DeviceSlotKind::USB, m_slot_id) != pad &&
                DeviceManager::instance().claim_assignable_device_if([&pad](const auto &device)
                                                                     { return device == pad; }))
            {
                m_profile->claimed_devices[{DeviceSlotKind::USB, m_slot_id}] = pad;
            }
            profile_mgr.set_xbox_one_auth_pending(m_profile->profile_id, m_list_id);
        }
    }
    else if (xbox_one)
    {
        // Still matching once the controller that authenticated is unplugged, as the console won't ask
        // again until it enumerates the device again
        matched = auth_broker.is_console_authenticated(ModeXboxOne) &&
                  profile_mgr.is_xbox_one_auth_holder(m_profile->profile_id, m_list_id);
    }
    if (!m_trigger && send_events && (matched != m_last_val || full_poll))
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, matched}}};
        HIDConfigDevice::send_event(event, true);
    }
    m_last_val = matched;
    return matched;
}
