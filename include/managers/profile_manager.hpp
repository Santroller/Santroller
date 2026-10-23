#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <cstddef>
#include <stdint.h>
#include <algorithm>
#include <optional>
#include "profiles/profile.hpp"
#include "instance.hpp"
#include "emulation/usb/device.hpp"
#include "config.pb.h"

class Device;
class UsbDevice;
class XboxOneGamepadDevice;

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
    std::shared_ptr<Profile> get_profile(uint32_t profile_id, size_t instance_id = 0);
    // Like get_profile, but builds the whole profile if it is inactive so the config tool can poll it
    std::shared_ptr<Profile> load_tool_profile(uint32_t profile_id, size_t instance_id);
    // Put the profile loaded for the tool back to only its triggers
    void release_tool_profile();
    
    void update_device_assignments(bool full_poll, bool send_events);
    void update(bool full_poll, bool send_events);
    bool assign_profile_to_devices(
        std::shared_ptr<Profile> profile,
        int assigned_devices,
        ConsoleMode usb_mode
    );
    bool changed_types();
    
    void register_instance(std::shared_ptr<Instance> instance, std::shared_ptr<Profile> profile, bool usb_instance);
    void remove_instance(std::shared_ptr<Instance> instance);
    
    bool has_active_instances() const;
    template <typename Func>
    void for_each_active_profile(Func func) const
    {
        for (const auto &entry : m_profile_to_instance)
        {
            for (const auto &instance : entry.second)
            {
                for (const auto &profile : instance->profiles)
                {
                    if (profile && profile->profile_id == entry.first)
                    {
                        func(entry.first, profile);
                    }
                }
            }
        }
    }
    // Calls func(profile_id, profile) once per physical Profile instance -
    // a profile_id can have more than one instance if multiple devices matched it.
    template <typename Func>
    void for_each_profile(Func func) const
    {
        for (const auto &entry : m_profiles)
        {
            for (const auto &profile : entry.second)
            {
                func(entry.first, profile);
            }
        }
    }
    
    void update_all_profile_devices(bool profile_changed, bool send_events);
    void update_profile_components(uint32_t profile_id, size_t instance_id, bool profile_changed, bool send_events);
    
    bool is_profile_active(uint32_t profile_id) const;
    std::vector<std::shared_ptr<Instance>> get_instances_for_profile(uint32_t profile_id) const
    {
        auto it = m_profile_to_instance.find(profile_id);
        if (it != m_profile_to_instance.end())
        {
            return it->second;
        }
        return {};
    }
    void clear_all();
    void prepare_for_config_reload();
    void finish_config_reload();
    std::shared_ptr<UsbDevice> reuse_usb_instance(uint8_t id, ConsoleMode mode, SubType subtype, bool auxiliary);
    void set_usb_reload_identity(uint8_t id, ConsoleMode mode, SubType subtype, bool auxiliary);
    void finish_usb_instance_initialization(uint8_t id);
    void initialize_device_bluetooth();
    void deinitialize_device_bluetooth();
    // Stops the config only bluetooth peripheral, so a bluetooth instance can take over the stack
    void stop_bt_config_instance();
    // The config only bluetooth peripheral outlives reloads, so it's told when the device name changes
    void refresh_bt_config_name();
    // The bluetooth gamepad from before the reload, if the new config wants one with the same GATT database and
    // report map (ie the same subtype). Reusing it keeps a connected host (or the configurator) connected through
    // the reload. Otherwise it's dropped, so it disconnects before a new one takes the stack.
    std::shared_ptr<Instance> reuse_bt_gamepad_instance(SubType subtype);
    void set_bt_gamepad_instance(std::shared_ptr<Instance> instance) { m_bt_gamepad_instance = instance; }
    // The emulated wii remote from before the reload. Unlike the gamepad it's reused whatever the subtype, as
    // that only changes the extension plugged into it.
    std::shared_ptr<Instance> reuse_wiimote_instance();
    void set_wiimote_instance(std::shared_ptr<Instance> instance) { m_wiimote_instance = instance; }
    // For something else taking the bluetooth stack
    void release_previous_bt_gamepad_instance() { m_previous_bt_gamepad_instance.reset(); }
    void release_previous_wiimote_instance() { m_previous_wiimote_instance.reset(); }
    
    void add_instance(std::shared_ptr<Instance> instance);
    size_t instance_count() const;
    template <typename Func>
    void for_each_instance(Func func) const
    {
        for (const auto &instance : m_instances)
        {
            if (instance)
            {
                func(instance);
            }
        }
    }
    size_t usb_instance_count() const;
    // Gamepads profiles are emulating over USB, not counting auxiliary interfaces (eg xinput security)
    size_t usb_gamepad_count() const { return m_current_subtypes.size(); }
    // Identifies the USB layout for windows, which keys driver bindings on the serial. 0 for a single HID
    // gamepad (or nothing), which keeps the plain serial
    uint32_t usb_layout_hash() const;
    // Windows with several XInput gamepads gets them as the slots of a 360 wireless receiver, as Steam's xbox
    // driver only takes one wired controller per USB device. Decided once a config has loaded, so the
    // descriptors can't change partway through an enumeration
    void update_xinput_receiver();
    bool xinput_receiver() const { return m_xinput_receiver; }
    // The rule an Xbox One controller is authenticating for, which becomes the holder once the console has
    // finished, and stays matched after the controller is unplugged
    void set_xbox_one_auth_pending(uint32_t profile_id, uint32_t list_id) { m_xbox_one_auth_pending = {profile_id, list_id}; }
    void commit_xbox_one_auth_holder() { m_xbox_one_auth_holder = m_xbox_one_auth_pending; }
    bool is_xbox_one_auth_holder(uint32_t profile_id, uint32_t list_id) const
    {
        return m_xbox_one_auth_holder && *m_xbox_one_auth_holder == std::make_pair(profile_id, list_id);
    }
    // Works out what the Xbox One device announces itself as, once a config has loaded all its profiles
    void update_xbox_one_identity();
    std::shared_ptr<UsbDevice> get_usb_instance(uint8_t id);
    void set_usb_instance(uint8_t id, std::shared_ptr<UsbDevice> instance);
    template <typename Func>
    void for_each_usb_instance(Func func) const
    {
        // a device with several interfaces fills a slot for each of them, but is only visited once
        const UsbDevice *last = nullptr;
        for (const auto &instance : m_usb_instances)
        {
            if (instance && instance.get() != last)
            {
                func(instance);
            }
            last = instance.get();
        }
    }
    // Whether any profile on USB wants to jump to PS3 mode when RPCS3 probes it. RPCS3's passthrough only
    // takes a single controller, so never with several gamepads on USB
    bool ps3_on_rpcs3() const;
    std::shared_ptr<UsbDevice> get_usb_instance_by_epin(uint8_t ep);
    void set_usb_instance_by_epin(uint8_t ep, std::shared_ptr<UsbDevice> instance);
    std::shared_ptr<UsbDevice> get_usb_instance_by_epout(uint8_t ep);
    void set_usb_instance_by_epout(uint8_t ep, std::shared_ptr<UsbDevice> instance);
    void map_usb_instance_epin(uint8_t ep, uint8_t interface_id);
    void map_usb_instance_epout(uint8_t ep, uint8_t interface_id);
    std::shared_ptr<UsbDevice> get_emulated_device(ConsoleMode mode);
    void set_emulated_device(ConsoleMode mode, std::shared_ptr<UsbDevice> device);

private:
    // Keeps the bluetooth config service reachable while no profile is using bluetooth
    void update_bt_config_instance(bool full_poll, bool send_events);

    ProfileManager() = default;
    ~ProfileManager() = default;
    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;

    std::unordered_map<uint32_t, std::vector<std::shared_ptr<Profile>>> m_profiles;
    std::vector<std::shared_ptr<Instance>> m_active_instances;
    std::vector<SubType> m_last_subtypes;
    std::vector<SubType> m_current_subtypes;
    bool m_subtypes_changed = false;
    std::optional<std::pair<uint32_t, uint32_t>> m_xbox_one_auth_holder;
    std::optional<std::pair<uint32_t, uint32_t>> m_xbox_one_auth_pending;
    bool m_xbox_one_identity_changed = false;
    bool m_xinput_receiver = false;
    bool m_was_xinput_receiver = false;
    std::unordered_map<uint32_t, std::vector<std::shared_ptr<Instance>>> m_profile_to_instance;
    // The inactive profile load_tool_profile last tried to build, even if that failed
    std::optional<uint32_t> m_tool_profile_id;
    
    std::vector<std::shared_ptr<Instance>> m_instances;
    std::shared_ptr<Instance> m_bt_config_instance;
    std::shared_ptr<Instance> m_bt_gamepad_instance;
    // Held from prepare_for_config_reload until the new config reuses it or something else needs the stack
    std::shared_ptr<Instance> m_previous_bt_gamepad_instance;
    std::shared_ptr<Instance> m_wiimote_instance;
    std::shared_ptr<Instance> m_previous_wiimote_instance;
    std::shared_ptr<UsbDevice> m_usb_instances[32];
    std::shared_ptr<UsbDevice> m_usb_instances_by_epin[16];
    std::shared_ptr<UsbDevice> m_usb_instances_by_epout[16];
    struct UsbReloadIdentity
    {
        ConsoleMode mode = ModeHid;
        SubType subtype = static_cast<SubType>(0);
        bool auxiliary = false;
        bool valid = false;
        UsbDevice::AllocationState before = {};
        UsbDevice::AllocationState after = {};
    };
    UsbReloadIdentity m_usb_reload_identities[32] = {};
    UsbReloadIdentity m_previous_usb_reload_identities[32] = {};
    std::shared_ptr<UsbDevice> m_previous_usb_instances[32];
    std::shared_ptr<UsbDevice> m_previous_usb_instances_by_epin[16];
    std::shared_ptr<UsbDevice> m_previous_usb_instances_by_epout[16];
    std::map<ConsoleMode, std::shared_ptr<UsbDevice>> m_emulated_devices;
};
