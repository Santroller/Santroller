#include "triggers/device_type_triggers.hpp"
#include "managers/device_manager.hpp"
#include "managers/profile_manager.hpp"
#include "managers/config_manager.hpp"

#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "protocols/midi_input.hpp"
#include "emulation/usb/hid_device.h"
#include <algorithm>

static void claim_profile_device(Profile *profile, DeviceSlotKind kind, uint16_t slot_id, const std::shared_ptr<Device> &device)
{
    printf("claim p=%d k=%d s=%d dev=%d\r\n", profile->profile_id, static_cast<int>(kind), slot_id, device->m_id);
    profile->claimed_devices[{kind, slot_id}] = device;
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
static void claim_profile_temp_device(Profile *profile, DeviceSlotKind kind, uint16_t slot_id, const std::shared_ptr<Device> &device)
{
    profile->temp_claimed_devices[{kind, slot_id}] = device;
}

static void clear_profile_temp_devices(Profile *profile)
{
    profile->temp_claimed_devices.clear();
}

// reads_claimed also looks at the devices other profiles have claimed, setting matched_claimed
// when that is where the device came from
template <typename Predicate>
static std::shared_ptr<Device> get_assignable_device(Profile *profile, bool claim_device, bool reads_claimed, bool &matched_claimed, Predicate predicate)
{
    matched_claimed = false;
    for (const auto &device : profile->claimed_devices)
    {
        if (device.second && device.second->still_connected && predicate(device.second))
        {
            return device.second;
        }
    }
    // Standby lighting alongside other active profiles only borrows controllers, so they stay where they are
    if (claim_device && !profile->standby_borrows)
    {
        return DeviceManager::instance().claim_assignable_device_if(predicate);
    }
    auto device = DeviceManager::instance().find_assignable_device_if(predicate);
    if (device || !(reads_claimed || profile->standby_borrows))
    {
        return device;
    }
    ProfileManager::instance().for_each_profile([&](uint32_t, const std::shared_ptr<Profile> &other)
                                                {
        if (device || !other || other.get() == profile)
        {
            return;
        }
        for (const auto &claimed : other->claimed_devices)
        {
            if (claimed.second && claimed.second->still_connected && predicate(claimed.second))
            {
                device = claimed.second;
                return;
            }
        } });
    matched_claimed = device != nullptr;
    return device;
}

static void send_activation_event(uint16_t id, uint16_t list_id, uint16_t analog_val, bool state)
{
    proto_Event event = {which_event : proto_Event_trigger_tag, event : {trigger : {id, list_id, analog_val, state}}};
    HIDConfigDevice::send_event(event, true);
}

WiiExtTypeActivationTrigger::WiiExtTypeActivationTrigger(proto_WiiExtType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_type(type)
{
}

bool WiiExtTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
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
            claim_profile_device(m_profile, DeviceSlotKind::WiiExtension, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::WiiExtension, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void WiiExtTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

PS2ControllerTypeActivationTrigger::PS2ControllerTypeActivationTrigger(proto_PS2ControllerType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_type(type)
{
}

bool PS2ControllerTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
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
            claim_profile_device(m_profile, DeviceSlotKind::PS2, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::PS2, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void PS2ControllerTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

SNESControllerTypeActivationTrigger::SNESControllerTypeActivationTrigger(proto_SNESControllerType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_type(type)
{
}

bool SNESControllerTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
                                        { return device->is_snes_type(m_type); });
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
            claim_profile_device(m_profile, DeviceSlotKind::SNES, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::SNES, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void SNESControllerTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

JoybusControllerTypeActivationTrigger::JoybusControllerTypeActivationTrigger(proto_JoybusControllerType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_type(type)
{
}

bool JoybusControllerTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
                                        { return device->is_joybus_type(m_type); });
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
            claim_profile_device(m_profile, DeviceSlotKind::Joybus, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::Joybus, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void JoybusControllerTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

UsbTypeActivationTrigger::UsbTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_type(type)
{
}

bool UsbTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
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
            claim_profile_device(m_profile, DeviceSlotKind::USB, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::USB, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void UsbTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

SpecificUsbDeviceActivationTrigger::SpecificUsbDeviceActivationTrigger(proto_SpecificUsbDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_device(device)
{
}

bool SpecificUsbDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
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
            claim_profile_device(m_profile, DeviceSlotKind::USB, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::USB, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void SpecificUsbDeviceActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

BluetoothTypeActivationTrigger::BluetoothTypeActivationTrigger(proto_SubType type, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_type(type)
{
}

bool BluetoothTypeActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
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
            claim_profile_device(m_profile, DeviceSlotKind::Bluetooth, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::Bluetooth, m_slot_id, device);
        }
        return true;
    }
    return false;
}

void BluetoothTypeActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}
SpecificBluetoothDeviceActivationTrigger::SpecificBluetoothDeviceActivationTrigger(proto_SpecificBluetoothDevice device, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_device(device)
{
}

bool SpecificBluetoothDeviceActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
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
            claim_profile_device(m_profile, DeviceSlotKind::Bluetooth, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::Bluetooth, m_slot_id, device);
        }
        return true;
    }
    return false;
}
void SpecificBluetoothDeviceActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

// Whether a MIDI slot limited to source can take device
static bool is_midi_source_device(const std::shared_ptr<Device> &device, const proto_MidiSourceDevice &source)
{
    switch (source.which_device)
    {
    case proto_MidiSourceDevice_usbDevice_tag:
        return device->is_usb_device(source.device.usbDevice);
    case proto_MidiSourceDevice_bluetoothDevice_tag:
        return device->is_bluetooth_device(source.device.bluetoothDevice);
    case proto_MidiSourceDevice_deviceid_tag:
        return device->m_id == source.device.deviceid;
    case proto_MidiSourceDevice_usbHost_tag:
        return device->is_usb_host_interface();
    case proto_MidiSourceDevice_bluetooth_tag:
        return device->is_bluetooth_host_interface();
    default:
        return true;
    }
}

MidiChannelActivationTrigger::MidiChannelActivationTrigger(uint32_t channel, const proto_MidiSourceDevice *source, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_channel(channel), m_has_source(source != nullptr), m_source(source ? *source : proto_MidiSourceDevice{})
{
}

bool MidiChannelActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
                                        { return device->has_midi_channel(m_channel - 1) && (!m_has_source || is_midi_source_device(device, m_source)); });
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
            claim_profile_device(m_profile, DeviceSlotKind::MIDI, m_slot_id, device);
            printf("Claimed device: %p %p\r\n", (void *)m_profile, (void *)device.get());
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::MIDI, m_slot_id, device);
        }
        return true;
    }
    return false;
}

void MidiChannelActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

MidiProGuitarActivationTrigger::MidiProGuitarActivationTrigger(proto_SubType type, const proto_MidiSourceDevice *source, std::shared_ptr<Profile> profile, uint32_t id, uint32_t slot_id, uint32_t list_id) : ActivationTrigger(profile, id, slot_id, list_id), m_channel(type == ProGuitarSquier ? MIDI_CHANNEL_PROGUITAR_SQUIER : MIDI_CHANNEL_PROGUITAR_MUSTANG), m_has_source(source != nullptr), m_source(source ? *source : proto_MidiSourceDevice{})
{
}

bool MidiProGuitarActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto device = get_assignable_device(m_profile, claim_device, m_reads_claimed, m_matched_claimed, [this](const auto &device)
                                        { return device->has_midi_channel(m_channel) && (!m_has_source || is_midi_source_device(device, m_source)); });
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
            claim_profile_device(m_profile, DeviceSlotKind::MIDI, m_slot_id, device);
        }
        else
        {
            claim_profile_temp_device(m_profile, DeviceSlotKind::MIDI, m_slot_id, device);
        }
        return true;
    }
    return false;
}

void MidiProGuitarActivationTrigger::reset()
{
    clear_profile_temp_devices(m_profile);
}

LocalInputsActivationTrigger::LocalInputsActivationTrigger(std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile, id, list_id)
{
}

bool LocalInputsActivationTrigger::validate(bool claim_device, bool full_poll, bool send_events)
{
    auto &config_mgr = ConfigManager::instance();
    bool matched = config_mgr.local_inputs_available_to(m_profile);
    if (send_events && (matched != m_last_val || full_poll))
    {
        m_last_val = matched;
        send_activation_event(m_id, m_list_id, m_last_analog_val, m_last_val);
    }
    else
    {
        m_last_val = matched;
    }
    if (matched && claim_device)
    {
        config_mgr.claim_local_inputs(m_profile);
    }
    return matched;
}
