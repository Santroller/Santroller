#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <cstddef>
#include <stdint.h>
#include "config.pb.h"

class Device;
class UsbHostInterface;

class DeviceManager
{
public:
    static DeviceManager& instance()
    {
        static DeviceManager manager;
        return manager;
    }

    // Active devices management
    void add_active_device(std::shared_ptr<Device> device);
    void remove_active_device(std::shared_ptr<Device> device);
    void clear_active_devices();
    template <typename Func>
    void for_each_active_device(Func func)
    {
        for (const auto &device : m_active_devices)
        {
            func(device);
        }
    }
    
    // Root devices (by ID)
    std::shared_ptr<Device> get_root_device(uint32_t id);
    void set_root_device(uint32_t id, std::shared_ptr<Device> device);
    void remove_root_device(uint32_t id);
    void mark_root_devices_disconnected();
    void remove_disconnected_root_devices();
    
    // Assignable devices
    void add_assignable_device(std::shared_ptr<Device> device);
    void clear_assignable_devices();
    size_t assignable_device_count() const;
    std::shared_ptr<Device> last_assignable_device() const;
    template <typename Predicate>
    std::shared_ptr<Device> find_assignable_device_if(Predicate predicate) const
    {
        for (const auto &device : m_assignable_devices)
        {
            if (predicate(device))
            {
                return device;
            }
        }
        return nullptr;
    }
    template <typename Predicate>
    std::shared_ptr<Device> claim_assignable_device_if(Predicate predicate)
    {
        auto it = m_assignable_devices.begin();
        while (it != m_assignable_devices.end())
        {
            auto device = *it;
            if (predicate(device))
            {
                m_assignable_devices.erase(it);
                return device;
            }
            ++it;
        }
        return nullptr;
    }
    template <typename Func>
    void for_each_assignable_device(Func func)
    {
        for (const auto &device : m_assignable_devices)
        {
            func(device);
        }
    }
    
    // Auth devices (by console mode)
    std::shared_ptr<UsbHostInterface> get_auth_device(ConsoleMode mode);
    void set_auth_device(ConsoleMode mode, std::shared_ptr<UsbHostInterface> device);
    void clear_auth_devices();
    void remove_device(Device *device);
    
    void update(bool profile_changed, bool send_events);
    void update_all_devices(bool profile_changed, bool send_events);
    void clear_all();

private:
    DeviceManager() = default;
    ~DeviceManager() = default;
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    std::vector<std::shared_ptr<Device>> m_active_devices;
    std::unordered_map<uint32_t, std::shared_ptr<Device>> m_root_devices;
    std::vector<std::shared_ptr<Device>> m_assignable_devices;
    std::map<ConsoleMode, std::shared_ptr<UsbHostInterface>> m_auth_devices;
};
