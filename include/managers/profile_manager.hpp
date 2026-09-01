#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <cstddef>
#include <stdint.h>
#include "profiles/profile.hpp"
#include "instance.hpp"
#include "config.pb.h"

class Device;
class UsbDevice;
struct EmulationDeviceConfig;

class ProfileManager
{
public:
    static ProfileManager& instance()
    {
        static ProfileManager manager;
        return manager;
    }

    void add_profile(uint32_t profile_id, std::shared_ptr<Profile> profile);
    void remove_profile(uint32_t profile_id);
    std::shared_ptr<Profile> get_profile(uint32_t profile_id);
    
    void update_device_assignments(bool full_poll, bool send_events);
    void update_active_instances();
    void update(bool full_poll, bool send_events);
    bool assign_profile_to_devices(
        std::shared_ptr<Profile> profile,
        int assigned_devices,
        ConsoleMode usb_mode,
        const EmulationDeviceConfig& emulation_devices
    );
    
    void register_instance(std::shared_ptr<Instance> instance, std::shared_ptr<Profile> profile);
    void remove_instance(std::shared_ptr<Instance> instance);
    
    bool has_active_instances() const;
    template <typename Func>
    void for_each_profile(Func func) const
    {
        for (const auto &profile : m_profiles)
        {
            func(profile.first, profile.second);
        }
    }
    
    void update_all_profile_devices(bool profile_changed, bool send_events);
    void update_profile_components(uint32_t profile_id, bool profile_changed, bool send_events);
    
    bool is_profile_active(uint32_t profile_id) const;
    void clear_all();
    void prepare_for_config_reload();
    bool has_previous_types() const;
    
    void add_instance(std::shared_ptr<Instance> instance);
    size_t instance_count() const;
    std::shared_ptr<UsbDevice> get_usb_instance(uint8_t id);
    void set_usb_instance(uint8_t id, std::shared_ptr<UsbDevice> instance);
    template <typename Func>
    void for_each_usb_instance(Func func) const
    {
        for (const auto &instance : m_usb_instances)
        {
            if (instance)
            {
                func(instance);
            }
        }
    }
    std::shared_ptr<UsbDevice> get_usb_instance_by_epin(uint8_t ep);
    void set_usb_instance_by_epin(uint8_t ep, std::shared_ptr<UsbDevice> instance);
    std::shared_ptr<UsbDevice> get_usb_instance_by_epout(uint8_t ep);
    void set_usb_instance_by_epout(uint8_t ep, std::shared_ptr<UsbDevice> instance);
    void map_usb_instance_epin(uint8_t ep, uint8_t interface_id);
    void map_usb_instance_epout(uint8_t ep, uint8_t interface_id);
    std::shared_ptr<UsbDevice> get_emulated_device(ConsoleMode mode);
    void set_emulated_device(ConsoleMode mode, std::shared_ptr<UsbDevice> device);
    
    SubType get_previous_type(uint32_t profile_id);
    void set_previous_type(uint32_t profile_id, SubType type);
    void track_profile_type(uint32_t profile_id, SubType type);
    SubType get_current_type(uint32_t profile_id);
    void set_current_type(uint32_t profile_id, SubType type);

private:
    ProfileManager() = default;
    ~ProfileManager() = default;
    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;

    std::unordered_map<uint32_t, std::shared_ptr<Profile>> m_profiles;
    std::vector<std::shared_ptr<Instance>> m_active_instances;
    std::unordered_map<uint32_t, std::shared_ptr<Instance>> m_profile_to_instance;
    
    std::vector<std::shared_ptr<Instance>> m_instances;
    std::shared_ptr<UsbDevice> m_usb_instances[32];
    std::shared_ptr<UsbDevice> m_usb_instances_by_epin[16];
    std::shared_ptr<UsbDevice> m_usb_instances_by_epout[16];
    std::map<ConsoleMode, std::shared_ptr<UsbDevice>> m_emulated_devices;
    std::unordered_map<uint32_t, SubType> m_prev_types;
    std::unordered_map<uint32_t, SubType> m_current_types;
};
