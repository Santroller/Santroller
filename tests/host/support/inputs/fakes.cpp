// Link time fakes for what the inputs, devices, triggers and profile call outside those sources
#include "config/config.hpp"
#include "main.hpp"
#include "utils.h"
#include "managers/config_manager.hpp"
#include "managers/device_manager.hpp"
#include "leds/led_mappings.hpp"
#include "fake_devices.hpp"

void reload()
{
    fake_config::reload_count++;
}

void update_aux_cycle(uint32_t id, uint32_t state)
{
    fake_config::aux_cycles.emplace_back(id, state);
}

void update_aux_toggle(uint32_t id, bool state)
{
    fake_config::aux_toggles.emplace_back(id, state);
}

// Same as src/main.cpp
bool mode_recently_changed()
{
    return ConfigManager::instance().mode_recently_changed(millis());
}

// Same as src/managers/device_manager.cpp
void DeviceManager::add_assignable_device(std::shared_ptr<Device> device)
{
    m_assignable_devices.push_back(device);
}

size_t DeviceManager::assignable_device_count() const
{
    return m_assignable_devices.size();
}

void DeviceManager::clear_assignable_devices()
{
    m_assignable_devices.clear();
}

// The real ones ask the emulation devices, which need the PIO / I2C slave hardware
bool DeviceManager::is_psx_communicating() const
{
    return fake_devices::psx_communicating;
}

bool DeviceManager::is_wii_communicating() const
{
    return fake_devices::wii_communicating;
}

bool DeviceManager::is_joybus_communicating() const
{
    return fake_devices::joybus_communicating;
}

bool DeviceManager::is_snes_communicating() const
{
    return fake_devices::snes_communicating;
}

// Same as src/leds/led_devices.cpp
void LedMapping::off()
{
    m_device->off();
}
