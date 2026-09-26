#include "triggers/device_type_triggers.hpp"
#include "managers/device_manager.hpp"
#include "managers/profile_manager.hpp"

#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include <algorithm>

static void claim_profile_device(Profile *profile, const std::shared_ptr<Device> &device)
{
    printf("Claiming profile device: profile=%d (%p) device=%d (%p)\n", profile->profile_id, profile, device->m_id, device.get());
    profile->devices[device->m_id] = device;
    auto instances = ProfileManager::instance().get_instances_for_profile(profile->profile_id);
    for (const auto &inst : instances)
    {
        if (inst)
        {
            device->set_player_led(inst->player_led);
            device->set_rumble(inst->rumble_left, inst->rumble_right);
            device->set_lightbar(inst->lightbar_red, inst->lightbar_green, inst->lightbar_blue);
            device->set_euphoria_led(inst->euphoria_led > 0);
            inst->update_capabilities();
        }
    }
}
static void claim_profile_temp_device(Profile *profile, const std::shared_ptr<Device> &device)
{
    profile->temp_devices[device->m_id] = device;
}

static void clear_profile_temp_devices(Profile *profile)
{
    profile->temp_devices.clear();
}

template <typename Predicate>
static std::shared_ptr<Device> get_assignable_device(Profile *profile, bool claim_device, Predicate predicate)
{
    for (const auto &device : profile->devices)
    {
        if (device.second && device.second->still_connected && predicate(device.second))
        {
            return device.second;
        }
    }
    if (claim_device)
    {
        return DeviceManager::instance().claim_assignable_device_if(predicate);
    }
    return DeviceManager::instance().find_assignable_device_if(predicate);
}

static void send_activation_event(uint16_t id, uint16_t list_id, uint16_t analog_val, bool state)
{
    proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {id, list_id, analog_val, state}}};
    HIDConfigDevice::send_event(event, true);
}

WiiExtTypeActivationTrigger::WiiExtTypeActivationTrigger(proto_WiiExtType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool WiiExtTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->is_wii_extension(m_type); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
        }
        return true;
    }
    return false;
}

PS2ControllerTypeActivationTrigger::PS2ControllerTypeActivationTrigger(proto_PS2ControllerType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool PS2ControllerTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->is_ps2_device(m_type); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
        }
        return true;
    }
    return false;
}

UsbTypeActivationTrigger::UsbTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool UsbTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->is_usb_type(m_type); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, device);
        }
        return true;
    }
    return false;
}
void UsbTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

SpecificUsbDeviceActivationTrigger::SpecificUsbDeviceActivationTrigger(proto_SpecificUsbDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_device(device)
{
}

bool SpecificUsbDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->is_usb_device(m_device); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, device);
        }
        return true;
    }
    return false;
}
void SpecificUsbDeviceActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

BluetoothTypeActivationTrigger::BluetoothTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_type(type)
{
}

bool BluetoothTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->is_bluetooth_type(m_type); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, device);
        }
        return true;
    }
    return false;
}

void BluetoothTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}
SpecificBluetoothDeviceActivationTrigger::SpecificBluetoothDeviceActivationTrigger(proto_SpecificBluetoothDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_device(device)
{
}

bool SpecificBluetoothDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->is_bluetooth_device(m_device); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, device);
        }
        return true;
    }
    return false;
}
void SpecificBluetoothDeviceActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

MidiChannelActivationTrigger::MidiChannelActivationTrigger(uint32_t channel, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id), m_channel(channel)
{
}

bool MidiChannelActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, [this](const auto &device)
                                        { return device->has_midi_channel(m_channel - 1); });
    bool matched = device != nullptr;
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (device)
    {
        if (claim_device)
        {
            claim_profile_device(m_profile, device);
            printf("Claimed device: %d %p %p\r\n", m_profile, m_profile, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, device);
        }
        return true;
    }
    return false;
}

void MidiChannelActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}
