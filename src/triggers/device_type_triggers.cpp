#include "triggers/device_type_triggers.hpp"
#include "managers/device_manager.hpp"

#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"

template <typename Predicate>
static std::shared_ptr<Device> get_assignable_device(bool claim_device, Predicate predicate)
{
    if (claim_device)
    {
        return DeviceManager::instance().claim_assignable_device_if(predicate);
    }
    return DeviceManager::instance().find_assignable_device_if(predicate);
}

WiiExtTypeActivationTrigger::WiiExtTypeActivationTrigger(proto_WiiExtType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool WiiExtTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->is_wii_extension(m_type);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices[device->m_id] = device;
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

PS2ControllerTypeActivationTrigger::PS2ControllerTypeActivationTrigger(proto_PS2ControllerType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool PS2ControllerTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->is_ps2_device(m_type);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices[device->m_id] = device;
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

UsbTypeActivationTrigger::UsbTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool UsbTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->is_usb_type(m_type);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices.insert_or_assign(device->m_id, device);
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

SpecificUsbDeviceActivationTrigger::SpecificUsbDeviceActivationTrigger(proto_SpecificUsbDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_device(device)
{
}

bool SpecificUsbDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->is_usb_device(m_device);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices[device->m_id] = device;
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

BluetoothTypeActivationTrigger::BluetoothTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool BluetoothTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->is_bluetooth_type(m_type);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices[device->m_id] = device;
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

SpecificBluetoothDeviceActivationTrigger::SpecificBluetoothDeviceActivationTrigger(proto_SpecificUsbDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_device(device)
{
}

bool SpecificBluetoothDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->is_bluetooth_device(m_device);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices[device->m_id] = device;
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}

MidiChannelActivationTrigger::MidiChannelActivationTrigger(uint32_t channel, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_channel(channel)
{
}

bool MidiChannelActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(claim_device, [this](const auto &device)
    {
        return device->has_midi_channel(m_channel - 1);
    });
    if (device)
    {
        if (claim_device)
        {
            m_profile->devices[device->m_id] = device;
            printf("Claimed device: %d %p %p\r\n", m_profile, m_profile, device);
        }
        m_last_val = true;
        return true;
    }
    if (send_events && full_poll)
    {
        proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {m_id, m_list_id, m_last_analog_val, m_last_val}}};
        HIDConfigDevice::send_event(event, true);
    }
    return false;
}
