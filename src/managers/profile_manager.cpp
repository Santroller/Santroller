#include "managers/profile_manager.hpp"
#include "config/config.hpp"
#include "config/emulation_device_config.hpp"
#include "config/instance_factory.hpp"
#include "managers/config_manager.hpp"
#include "devices/base.hpp"
#include "mappings/mapping.hpp"
#include "leds/led_mappings.hpp"
#include "triggers/activation_trigger.hpp"
#include <algorithm>

namespace
{
void release_profile_contents(std::unordered_map<uint32_t, std::shared_ptr<Profile>>& profiles)
{
    for (auto& profile_pair : profiles)
    {
        auto& profile = profile_pair.second;
        profile->mappings.clear();
        profile->triggers.clear();
        profile->leds.clear();
        profile->devices.clear();
    }
}
}

void ProfileManager::add_profile(uint32_t profile_id, std::shared_ptr<Profile> profile)
{
    m_profiles[profile_id] = profile;
}

void ProfileManager::remove_profile(uint32_t profile_id)
{
    auto it = m_profile_to_instance.find(profile_id);
    if (it != m_profile_to_instance.end())
    {
        remove_instance(it->second);
    }
    m_profiles.erase(profile_id);
}

std::shared_ptr<Profile> ProfileManager::get_profile(uint32_t profile_id)
{
    auto it = m_profiles.find(profile_id);
    return (it != m_profiles.end()) ? it->second : nullptr;
}

void ProfileManager::register_instance(std::shared_ptr<Instance> instance, std::shared_ptr<Profile> profile)
{
    m_active_instances.push_back(instance);
    m_profile_to_instance[profile->profile_id] = instance;
}

void ProfileManager::remove_instance(std::shared_ptr<Instance> instance)
{
    for (const auto& profile : instance->profiles)
    {
        m_profile_to_instance.erase(profile->profile_id);
        
        for (auto& device_pair : profile->devices)
        {
            device_pair.second->still_connected = false;
        }
        profile->devices.clear();
    }
    
    m_active_instances.erase(
        std::remove(m_active_instances.begin(), m_active_instances.end(), instance),
        m_active_instances.end()
    );
}

void ProfileManager::update_device_assignments(bool full_poll, bool send_events)
{
    for (auto& profile_pair : m_profiles)
    {
        auto& profile = profile_pair.second;
        
        for (auto& trigger_list : profile->triggers)
        {
            bool matched = trigger_list->validate(false, full_poll, send_events);
            
            if (matched)
            {
                trigger_list->validate(true, full_poll, send_events);
            }
        }
    }
}

void ProfileManager::update_active_instances()
{
    auto it = m_active_instances.begin();
    while (it != m_active_instances.end())
    {
        auto& instance = *it;
        bool has_devices = false;
        
        for (const auto& profile : instance->profiles)
        {
            if (!profile->devices.empty())
            {
                has_devices = true;
                break;
            }
        }
        
        if (!has_devices)
        {
            for (const auto& profile : instance->profiles)
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
    ConsoleMode usb_mode,
    const EmulationDeviceConfig& emulation_devices)
{
    bool assigned = false;
    auto& config_mgr = ConfigManager::instance();
    const int assignment_types[] = {
        ProfileAssignMask_AssignBluetoothGamepad,
        ProfileAssignMask_AssignPsx,
        ProfileAssignMask_AssignWiimoteExtension,
        ProfileAssignMask_AssignUsb
    };

    for (int assignment_type : assignment_types)
    {
        if (!(assigned_devices & assignment_type) || config_mgr.has_seen_assignment(assignment_type))
        {
            continue;
        }

        if (assignment_type == ProfileAssignMask_AssignBluetoothGamepad && !config_mgr.has_bluetooth())
        {
            continue;
        }

        auto instance = InstanceFactory::create_instance(assignment_type, profile, usb_mode, emulation_devices);
        if (instance)
        {
            config_mgr.mark_seen_assignment(assignment_type);
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
    m_instances.clear();
    m_active_instances.clear();
    release_profile_contents(m_profiles);
    m_profiles.clear();
    m_profile_to_instance.clear();
    m_prev_types = m_current_types;
    m_current_types.clear();
    m_emulated_devices.clear();
    std::fill(std::begin(m_usb_instances), std::end(m_usb_instances), nullptr);
    std::fill(std::begin(m_usb_instances_by_epin), std::end(m_usb_instances_by_epin), nullptr);
    std::fill(std::begin(m_usb_instances_by_epout), std::end(m_usb_instances_by_epout), nullptr);
}

bool ProfileManager::has_previous_types() const
{
    return !m_prev_types.empty();
}

void ProfileManager::update_all_profile_devices(bool profile_changed, bool send_events) {
    for (const auto &profile : m_profiles) {
        for (const auto &device : profile.second->devices) {
            if (device.second) {
                device.second->update(profile_changed, send_events);
            }
        }
    }
}

void ProfileManager::update_profile_components(uint32_t profile_id, bool profile_changed, bool send_events) {
    auto profile = get_profile(profile_id);
    if (!profile) {
        return;
    }
    
    // Update mappings
    for (const auto &mapping : profile->mappings) {
        mapping->update(profile_changed, send_events);
    }
    
    // Validate triggers
    for (const auto &trigger : profile->triggers) {
        trigger->validate(false, profile_changed, send_events);
    }
    
    // Update LEDs
    for (const auto &led : profile->leds) {
        led->update(profile_changed, send_events);
    }
}

void ProfileManager::clear_all() {
    m_instances.clear();
    m_active_instances.clear();
    release_profile_contents(m_profiles);
    m_profiles.clear();
    m_profile_to_instance.clear();
    m_prev_types.clear();
    m_current_types.clear();
    m_emulated_devices.clear();
    std::fill(std::begin(m_usb_instances), std::end(m_usb_instances), nullptr);
    std::fill(std::begin(m_usb_instances_by_epin), std::end(m_usb_instances_by_epin), nullptr);
    std::fill(std::begin(m_usb_instances_by_epout), std::end(m_usb_instances_by_epout), nullptr);
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

std::shared_ptr<UsbDevice> ProfileManager::get_usb_instance(uint8_t id)
{
    return (id < 32) ? m_usb_instances[id] : nullptr;
}

void ProfileManager::set_usb_instance(uint8_t id, std::shared_ptr<UsbDevice> instance)
{
    if (id < 32) m_usb_instances[id] = instance;
}

std::shared_ptr<UsbDevice> ProfileManager::get_usb_instance_by_epin(uint8_t ep)
{
    return (ep < 16) ? m_usb_instances_by_epin[ep] : nullptr;
}

void ProfileManager::set_usb_instance_by_epin(uint8_t ep, std::shared_ptr<UsbDevice> instance)
{
    if (ep < 16) m_usb_instances_by_epin[ep] = instance;
}

std::shared_ptr<UsbDevice> ProfileManager::get_usb_instance_by_epout(uint8_t ep)
{
    return (ep < 16) ? m_usb_instances_by_epout[ep] : nullptr;
}

void ProfileManager::set_usb_instance_by_epout(uint8_t ep, std::shared_ptr<UsbDevice> instance)
{
    if (ep < 16) m_usb_instances_by_epout[ep] = instance;
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

// Type tracking methods
SubType ProfileManager::get_previous_type(uint32_t profile_id)
{
    auto it = m_prev_types.find(profile_id);
    return (it != m_prev_types.end()) ? it->second : SubType_Gamepad;
}

void ProfileManager::set_previous_type(uint32_t profile_id, SubType type)
{
    m_prev_types[profile_id] = type;
}

void ProfileManager::track_profile_type(uint32_t profile_id, SubType type)
{
    auto match = m_prev_types.find(profile_id);
    if (match != m_prev_types.end())
    {
        if (match->second != type)
        {
            m_prev_types[profile_id] = type;
        }
    }
    else
    {
        m_prev_types.emplace(profile_id, type);
    }
}

SubType ProfileManager::get_current_type(uint32_t profile_id)
{
    auto it = m_current_types.find(profile_id);
    return (it != m_current_types.end()) ? it->second : SubType_Gamepad;
}

void ProfileManager::set_current_type(uint32_t profile_id, SubType type)
{
    m_current_types[profile_id] = type;
}
