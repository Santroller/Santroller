#include "managers/profile_manager.hpp"
#include "config/config.hpp"
#include "config/instance_factory.hpp"
#include "managers/config_manager.hpp"
#include "devices/base.hpp"
#include "mappings/mapping.hpp"
#include "leds/led_mappings.hpp"
#include "triggers/activation_trigger.hpp"
#include "emulation/usb/xone_device.h"
#include <algorithm>

namespace
{
    void release_profile_contents(std::unordered_map<uint32_t, std::vector<std::shared_ptr<Profile>>> &profiles)
    {
        for (auto &profile_pair : profiles)
        {
            for (auto &profile : profile_pair.second)
            {
                profile->mappings.clear();
                profile->triggers.clear();
                profile->leds.clear();
                profile->devices.clear();
                profile->claimed_devices.clear();
                profile->temp_claimed_devices.clear();
            }
        }
    }
}
bool ProfileManager::changed_types()
{
    printf("ProfileManager::changed_types() called with m_subtypes_changed=%d, m_current_subtypes.size()=%zu, m_last_subtypes.size()=%zu\n", m_subtypes_changed, m_current_subtypes.size(), m_last_subtypes.size());
    for (size_t i = 0; i < std::size(m_usb_instances); ++i)
    {
        if (m_usb_instances[i] != m_previous_usb_instances[i])
        {
            return true;
        }
    }
    auto dev = get_emulated_device(ModeXboxOne);
    if (m_was_legacy_adapter && dev &&
        std::static_pointer_cast<XboxOneGamepadDevice>(dev)->is_legacy_adapter())
    {
        return false;
    }
    return m_subtypes_changed || m_current_subtypes.size() < m_last_subtypes.size();
}
void ProfileManager::add_profile(uint32_t profile_id, std::shared_ptr<Profile> profile)
{
    if (profile)
    {
        profile->resolve_shortcuts();
    }
    m_profiles[profile_id].push_back(profile);
}

void ProfileManager::remove_profile(uint32_t profile_id)
{
    auto it = m_profile_to_instance.find(profile_id);
    if (it != m_profile_to_instance.end())
    {
        // copy first since remove_instance() mutates m_profile_to_instance
        auto instances = it->second;
        for (auto &instance : instances)
        {
            remove_instance(instance);
        }
    }
    m_profiles.erase(profile_id);
}

std::shared_ptr<Profile> ProfileManager::get_profile(uint32_t profile_id, size_t instance_id)
{
    auto instance_it = m_profile_to_instance.find(profile_id);
    if (instance_it != m_profile_to_instance.end() && !instance_it->second.empty())
    {
        if (instance_id >= instance_it->second.size())
        {
            instance_id = 0;
        }
        auto &instance = instance_it->second[instance_id];
        for (const auto &profile : instance->profiles)
        {
            if (profile && profile->profile_id == profile_id)
            {
                return profile;
            }
        }
    }

    // A profile_id can back multiple physical instances; return the requested or first as a representative.
    auto it = m_profiles.find(profile_id);
    if (it == m_profiles.end() || it->second.empty())
    {
        return nullptr;
    }
    if (instance_id < it->second.size())
    {
        return it->second[instance_id];
    }
    return it->second.front();
}

void ProfileManager::register_instance(std::shared_ptr<Instance> instance, std::shared_ptr<Profile> profile, bool usb_instance)
{
    // Only USB instances affect descriptors; PS2/Wii/Bluetooth assignments must not reset the PC link.
    if (usb_instance)
    {
        m_current_subtypes.push_back(instance->subtype);
        if (m_current_subtypes.size() > m_last_subtypes.size() || m_last_subtypes[m_current_subtypes.size() - 1] != instance->subtype)
        {
            m_subtypes_changed = true;
        }
    }
    if (std::find(m_active_instances.begin(), m_active_instances.end(), instance) == m_active_instances.end())
    {
        m_active_instances.push_back(instance);
    }
    m_profile_to_instance[profile->profile_id].push_back(instance);
}

void ProfileManager::remove_instance(std::shared_ptr<Instance> instance)
{
    for (const auto &profile : instance->profiles)
    {
        auto it = m_profile_to_instance.find(profile->profile_id);
        if (it != m_profile_to_instance.end())
        {
            auto &instances = it->second;
            instances.erase(std::remove(instances.begin(), instances.end(), instance), instances.end());
            if (instances.empty())
            {
                m_profile_to_instance.erase(it);
            }
        }

        for (auto &device_pair : profile->devices)
        {
            device_pair.second->still_connected = false;
        }
        profile->devices.clear();
        for (auto &device_pair : profile->claimed_devices)
        {
            device_pair.second->still_connected = false;
        }
        profile->claimed_devices.clear();
        profile->temp_claimed_devices.clear();
    }

    m_active_instances.erase(
        std::remove(m_active_instances.begin(), m_active_instances.end(), instance),
        m_active_instances.end());
}

void ProfileManager::update_device_assignments(bool full_poll, bool send_events)
{
    for (auto &profile_pair : m_profiles)
    {
        for (auto &profile : profile_pair.second)
        {
            bool has_claimed_list = false;
            for (auto &trigger_list : profile->triggers)
            {
                if (trigger_list->claimed())
                {
                    has_claimed_list = true;
                    break;
                }
            }

            for (auto &trigger_list : profile->triggers)
            {
                bool matched = trigger_list->validate(false, full_poll, send_events);

                if (matched && !has_claimed_list)
                {
                    trigger_list->validate(true, full_poll, send_events);
                    has_claimed_list = true;
                }
            }
        }
    }
}

void ProfileManager::update_active_instances()
{
    auto it = m_active_instances.begin();
    while (it != m_active_instances.end())
    {
        auto &instance = *it;
        bool has_devices = false;

        for (const auto &profile : instance->profiles)
        {
            if (!profile->devices.empty() || !profile->claimed_devices.empty())
            {
                has_devices = true;
                break;
            }
        }

        if (!has_devices)
        {
            for (const auto &profile : instance->profiles)
            {
                m_profile_to_instance.erase(profile->profile_id);
            }
            it = m_active_instances.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool ProfileManager::assign_profile_to_devices(
    std::shared_ptr<Profile> profile,
    int assigned_devices,
    ConsoleMode usb_mode)
{
    bool assigned = false;
    auto &config_mgr = ConfigManager::instance();
    const int assignment_types[] = {
        ProfileAssignMask_AssignBluetoothGamepad,
        ProfileAssignMask_AssignBluetoothWiimote,
        ProfileAssignMask_AssignPsx,
        ProfileAssignMask_AssignWiimoteExtension,
        ProfileAssignMask_AssignUsb};

    for (int assignment_type : assignment_types)
    {
        // USB is composite (multiple interfaces), so unlike the single-resource
        // assignments below it can be granted to more than one profile per boot.
        bool exclusive = assignment_type != ProfileAssignMask_AssignUsb;
        if (!(assigned_devices & assignment_type) || (exclusive && config_mgr.has_seen_assignment(assignment_type)))
        {
            continue;
        }

        if ((assignment_type == ProfileAssignMask_AssignBluetoothGamepad ||
             assignment_type == ProfileAssignMask_AssignBluetoothWiimote) &&
            !config_mgr.has_bluetooth())
        {
            continue;
        }
        printf("Attempting to assign profile %u to assignment type %d\n", profile->profile_id, assignment_type);

        auto instance = InstanceFactory::create_instance(assignment_type, profile, usb_mode);
        if (instance)
        {
            printf("Successfully assigned profile %u to assignment type %d\n", profile->profile_id, assignment_type);
            if (exclusive)
            {
                config_mgr.mark_seen_assignment(assignment_type);
            }
            assigned = true;
        }
    }

    return assigned;
}

void ProfileManager::update(bool full_poll, bool send_events)
{
    for (const auto &instance : m_instances)
    {
        instance->process(full_poll, send_events);
    }

    update_device_assignments(full_poll, send_events);
    update_active_instances();
}

bool ProfileManager::is_profile_active(uint32_t profile_id) const
{
    return m_profile_to_instance.find(profile_id) != m_profile_to_instance.end();
}

bool ProfileManager::has_active_instances() const
{
    return !m_active_instances.empty();
}

void ProfileManager::prepare_for_config_reload()
{
    m_last_subtypes = m_current_subtypes;
    m_current_subtypes.clear();
    m_subtypes_changed = false;
    auto xone = get_emulated_device(ModeXboxOne);
    m_was_legacy_adapter = xone && std::static_pointer_cast<XboxOneGamepadDevice>(xone)->is_legacy_adapter();
    std::copy(std::begin(m_usb_instances), std::end(m_usb_instances), std::begin(m_previous_usb_instances));
    std::copy(std::begin(m_usb_instances_by_epin), std::end(m_usb_instances_by_epin), std::begin(m_previous_usb_instances_by_epin));
    std::copy(std::begin(m_usb_instances_by_epout), std::end(m_usb_instances_by_epout), std::begin(m_previous_usb_instances_by_epout));
    std::copy(std::begin(m_usb_reload_identities), std::end(m_usb_reload_identities), std::begin(m_previous_usb_reload_identities));
    std::fill(std::begin(m_usb_reload_identities), std::end(m_usb_reload_identities), UsbReloadIdentity{});

    for (size_t i = 0; i < std::size(m_previous_usb_instances); ++i)
    {
        if (m_previous_usb_instances[i] && m_previous_usb_reload_identities[i].valid &&
            !m_previous_usb_reload_identities[i].auxiliary)
        {
            m_previous_usb_instances[i]->profiles.clear();
        }
    }

    m_instances.clear();
    m_active_instances.clear();
    release_profile_contents(m_profiles);
    m_profiles.clear();
    m_profile_to_instance.clear();
    m_emulated_devices.clear();
    std::fill(std::begin(m_usb_instances), std::end(m_usb_instances), nullptr);
    std::fill(std::begin(m_usb_instances_by_epin), std::end(m_usb_instances_by_epin), nullptr);
    std::fill(std::begin(m_usb_instances_by_epout), std::end(m_usb_instances_by_epout), nullptr);
}

void ProfileManager::finish_config_reload()
{
    // The old endpoint buffers must remain alive until the device stack has stopped using them.
    std::fill(std::begin(m_previous_usb_instances), std::end(m_previous_usb_instances), nullptr);
    std::fill(std::begin(m_previous_usb_instances_by_epin), std::end(m_previous_usb_instances_by_epin), nullptr);
    std::fill(std::begin(m_previous_usb_instances_by_epout), std::end(m_previous_usb_instances_by_epout), nullptr);
}

void ProfileManager::set_usb_reload_identity(uint8_t id, ConsoleMode mode, SubType subtype, bool auxiliary)
{
    if (id < std::size(m_usb_reload_identities))
    {
        m_usb_reload_identities[id] = {mode, subtype, auxiliary, true, UsbDevice::allocation_state(), {}};
    }
}

void ProfileManager::finish_usb_instance_initialization(uint8_t id)
{
    if (id < std::size(m_usb_reload_identities) && m_usb_reload_identities[id].valid)
    {
        m_usb_reload_identities[id].after = UsbDevice::allocation_state();
    }
}

std::shared_ptr<UsbDevice> ProfileManager::reuse_usb_instance(uint8_t id, ConsoleMode mode, SubType subtype, bool auxiliary)
{
    if (id >= std::size(m_previous_usb_instances))
    {
        return nullptr;
    }
    const auto &identity = m_previous_usb_reload_identities[id];
    if (!identity.valid || identity.mode != mode || identity.subtype != subtype ||
        identity.auxiliary != auxiliary || !m_previous_usb_instances[id])
    {
        return nullptr;
    }
    const auto current = UsbDevice::allocation_state();
    if (current.epin != identity.before.epin || current.epout != identity.before.epout ||
        current.strid != identity.before.strid)
    {
        return nullptr;
    }
    auto instance = m_previous_usb_instances[id];
    printf("Reusing USB instance at interface %u, mode %d, subtype %d\n", id, mode, subtype);
    m_instances.push_back(instance);
    m_usb_instances[id] = instance;
    m_usb_reload_identities[id] = identity;
    UsbDevice::restore_allocation_state(identity.after);
    for (size_t ep = 0; ep < std::size(m_usb_instances_by_epin); ++ep)
    {
        if (m_previous_usb_instances_by_epin[ep] == instance)
        {
            m_usb_instances_by_epin[ep] = instance;
        }
        if (m_previous_usb_instances_by_epout[ep] == instance)
        {
            m_usb_instances_by_epout[ep] = instance;
        }
    }
    return instance;
}

void ProfileManager::update_all_profile_devices(bool profile_changed, bool send_events)
{
    for (const auto &entry : m_profiles)
    {
        for (const auto &profile : entry.second)
        {
            for (const auto &device : profile->devices)
            {
                if (device.second)
                {
                    device.second->update(profile_changed, send_events);
                }
            }
            for (const auto &device : profile->claimed_devices)
            {
                if (device.second)
                {
                    device.second->update(profile_changed, send_events);
                }
            }
        }
    }
}

void ProfileManager::update_profile_components(uint32_t profile_id, size_t instance_id, bool profile_changed, bool send_events)
{
    auto profile = get_profile(profile_id, instance_id);
    if (!profile)
    {
        return;
    }

    // Update mappings
    for (const auto &mapping : profile->mappings)
    {
        mapping->update(profile_changed, send_events);
    }

    // Validate triggers
    for (const auto &trigger : profile->triggers)
    {
        trigger->validate(false, profile_changed, send_events);
    }

    // Update LEDs
    for (const auto &led : profile->leds)
    {
        led->update(profile_changed, send_events);
    }
}

void ProfileManager::clear_all()
{
    m_instances.clear();
    m_active_instances.clear();
    release_profile_contents(m_profiles);
    m_profiles.clear();
    m_profile_to_instance.clear();
    m_last_subtypes.clear();
    m_current_subtypes.clear();
    m_subtypes_changed = false;
    m_was_legacy_adapter = false;
    m_emulated_devices.clear();
    std::fill(std::begin(m_usb_instances), std::end(m_usb_instances), nullptr);
    std::fill(std::begin(m_usb_instances_by_epin), std::end(m_usb_instances_by_epin), nullptr);
    std::fill(std::begin(m_usb_instances_by_epout), std::end(m_usb_instances_by_epout), nullptr);
    std::fill(std::begin(m_usb_reload_identities), std::end(m_usb_reload_identities), UsbReloadIdentity{});
    finish_config_reload();
}

void ProfileManager::initialize_device_bluetooth()
{
    for (const auto &instance : m_instances)
    {
        if (instance && instance->is_bluetooth())
        {
            instance->initialize();
        }
    }
}

void ProfileManager::deinitialize_device_bluetooth()
{
    for (const auto &instance : m_instances)
    {
        if (instance && instance->is_bluetooth())
        {
            instance->deinitialize();
        }
    }
}

// Instance management methods
void ProfileManager::add_instance(std::shared_ptr<Instance> instance)
{
    m_instances.push_back(instance);
}

size_t ProfileManager::instance_count() const
{
    return m_instances.size();
}

size_t ProfileManager::usb_instance_count() const
{
    size_t count = 0;
    for (const auto &instance : m_usb_instances)
    {
        if (instance)
        {
            count++;
        }
    }
    return count;
}

std::shared_ptr<UsbDevice> ProfileManager::get_usb_instance(uint8_t id)
{
    return (id < 32) ? m_usb_instances[id] : nullptr;
}

void ProfileManager::set_usb_instance(uint8_t id, std::shared_ptr<UsbDevice> instance)
{
    if (id < 32)
        m_usb_instances[id] = instance;
}

std::shared_ptr<UsbDevice> ProfileManager::get_usb_instance_by_epin(uint8_t ep)
{
    return (ep < 16) ? m_usb_instances_by_epin[ep] : nullptr;
}

void ProfileManager::set_usb_instance_by_epin(uint8_t ep, std::shared_ptr<UsbDevice> instance)
{
    if (ep < 16)
        m_usb_instances_by_epin[ep] = instance;
}

std::shared_ptr<UsbDevice> ProfileManager::get_usb_instance_by_epout(uint8_t ep)
{
    return (ep < 16) ? m_usb_instances_by_epout[ep] : nullptr;
}

void ProfileManager::set_usb_instance_by_epout(uint8_t ep, std::shared_ptr<UsbDevice> instance)
{
    if (ep < 16)
        m_usb_instances_by_epout[ep] = instance;
}

void ProfileManager::map_usb_instance_epin(uint8_t ep, uint8_t interface_id)
{
    set_usb_instance_by_epin(ep & (~0x80), get_usb_instance(interface_id));
}

void ProfileManager::map_usb_instance_epout(uint8_t ep, uint8_t interface_id)
{
    set_usb_instance_by_epout(ep, get_usb_instance(interface_id));
}

std::shared_ptr<UsbDevice> ProfileManager::get_emulated_device(ConsoleMode mode)
{
    auto it = m_emulated_devices.find(mode);
    return (it != m_emulated_devices.end()) ? it->second : nullptr;
}

void ProfileManager::set_emulated_device(ConsoleMode mode, std::shared_ptr<UsbDevice> device)
{
    m_emulated_devices[mode] = device;
}
