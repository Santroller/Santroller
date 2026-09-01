#include "managers/device_manager.hpp"
#include "devices/usb/host/host.hpp"
#include "devices/base.hpp"
#include <algorithm>

namespace
{
template <typename DeviceType, typename Predicate>
void erase_devices_if(std::vector<std::shared_ptr<DeviceType>> &devices, Predicate predicate)
{
    devices.erase(std::remove_if(devices.begin(), devices.end(), predicate), devices.end());
}

void erase_usb_device(std::vector<std::shared_ptr<UsbHostInterface>> &devices, UsbHostInterface *device)
{
    erase_devices_if(devices, [device](const auto &candidate)
                     { return candidate.get() == device; });
}

void erase_usb_devices_by_address(std::vector<std::shared_ptr<UsbHostInterface>> &devices, uint8_t dev_addr)
{
    erase_devices_if(devices, [dev_addr](const auto &device)
                     { return device->dev_addr() == dev_addr; });
}
}

void DeviceManager::add_active_device(std::shared_ptr<Device> device)
{
    m_active_devices.push_back(device);
}

void DeviceManager::remove_active_device(std::shared_ptr<Device> device)
{
    m_active_devices.erase(
        std::remove(m_active_devices.begin(), m_active_devices.end(), device),
        m_active_devices.end()
    );
}

void DeviceManager::clear_active_devices()
{
    m_active_devices.clear();
}

std::shared_ptr<Device> DeviceManager::get_root_device(uint32_t id)
{
    auto it = m_root_devices.find(id);
    return (it != m_root_devices.end()) ? it->second : nullptr;
}

void DeviceManager::set_root_device(uint32_t id, std::shared_ptr<Device> device)
{
    m_root_devices[id] = device;
}

void DeviceManager::remove_root_device(uint32_t id)
{
    m_root_devices.erase(id);
}

void DeviceManager::mark_root_devices_disconnected()
{
    for (auto &device : m_root_devices)
    {
        device.second->still_connected = false;
    }
}

void DeviceManager::remove_disconnected_root_devices()
{
    auto it = m_root_devices.begin();
    while (it != m_root_devices.end())
    {
        if (!it->second->still_connected)
        {
            it->second->end(false);
            it = m_root_devices.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void DeviceManager::add_assignable_device(std::shared_ptr<Device> device)
{
    m_assignable_devices.push_back(device);
}

void DeviceManager::clear_assignable_devices()
{
    m_assignable_devices.clear();
}

std::shared_ptr<Device> DeviceManager::last_assignable_device() const
{
    return m_assignable_devices.empty() ? nullptr : m_assignable_devices.back();
}

void DeviceManager::add_assignable_usb_device(std::shared_ptr<UsbHostInterface> device)
{
    m_assignable_usb_devices.push_back(device);
}

void DeviceManager::add_enumerating_usb_device(std::shared_ptr<UsbHostInterface> device)
{
    m_enumerating_usb_devices.push_back(device);
}

void DeviceManager::remove_assignable_usb_device(UsbHostInterface *device)
{
    erase_usb_device(m_assignable_usb_devices, device);
}

void DeviceManager::remove_enumerating_usb_device(UsbHostInterface *device)
{
    erase_usb_device(m_enumerating_usb_devices, device);
}

void DeviceManager::remove_assignable_usb_devices_by_address(uint8_t dev_addr)
{
    erase_usb_devices_by_address(m_assignable_usb_devices, dev_addr);
}

void DeviceManager::remove_enumerating_usb_devices_by_address(uint8_t dev_addr)
{
    erase_usb_devices_by_address(m_enumerating_usb_devices, dev_addr);
}

size_t DeviceManager::assignable_usb_device_count() const
{
    return m_assignable_usb_devices.size();
}

size_t DeviceManager::enumerating_usb_device_count() const
{
    return m_enumerating_usb_devices.size();
}

void DeviceManager::add_assignable_devices_from_usb_hosts(bool rescan)
{
    for (const auto &device : m_assignable_usb_devices)
    {
        m_assignable_devices.push_back(device);
        if (rescan)
        {
            device->rescan(true);
        }
    }
}

void DeviceManager::clear_usb_devices()
{
    m_assignable_usb_devices.clear();
    m_enumerating_usb_devices.clear();
}

std::shared_ptr<UsbHostInterface> DeviceManager::get_auth_device(ConsoleMode mode)
{
    auto it = m_auth_devices.find(mode);
    return (it != m_auth_devices.end()) ? it->second : nullptr;
}

void DeviceManager::set_auth_device(ConsoleMode mode, std::shared_ptr<UsbHostInterface> device)
{
    m_auth_devices[mode] = device;
}

void DeviceManager::update(bool profile_changed, bool send_events) {
    for (const auto &device : m_active_devices) {
        device->update(profile_changed, send_events);
    }
    for (const auto &device : m_assignable_devices) {
        device->update(profile_changed, send_events);
    }
    for (const auto &device : m_enumerating_usb_devices) {
        device->update(profile_changed, send_events);
    }
    for (const auto &device : m_assignable_usb_devices) {
        device->update(profile_changed, send_events);
    }
}

void DeviceManager::update_all_devices(bool profile_changed, bool send_events) {
    update(profile_changed, send_events);
}

void DeviceManager::clear_auth_devices()
{
    m_auth_devices.clear();
}

void DeviceManager::clear_all()
{
    m_active_devices.clear();
    m_root_devices.clear();
    m_assignable_devices.clear();
    m_assignable_usb_devices.clear();
    m_enumerating_usb_devices.clear();
    m_auth_devices.clear();
}
