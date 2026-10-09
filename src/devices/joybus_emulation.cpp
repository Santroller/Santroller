#include "devices/joybus_emulation.hpp"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include <cstring>
#include <stdio.h>

JoybusEmulationDevice::JoybusEmulationDevice(const DeviceReloadState *state, proto_JoybusEmulationDevice device, uint16_t id)
    : Device(id), m_device(device), m_controller(device.dataPin, device.has_console && device.console == JoybusConsoleN64)
{
    if (state && state->valid)
    {
        m_last_communicating = state->joybus_emulation_communicating;
    }
}

bool JoybusEmulationDevice::matches_reload_config(const proto_Device &config) const
{
    return config.which_device == proto_Device_joybusEmulation_tag &&
           memcmp(&m_device, &config.device.joybusEmulation, sizeof(m_device)) == 0;
}

void JoybusEmulationDevice::save_reload_state(DeviceReloadState &state) const
{
    state.valid = true;
    state.joybus_emulation_communicating = m_last_communicating;
}

void JoybusEmulationDevice::begin()
{
    // Don't answer the console until a profile is assigned, just watch for it
    if (!m_controller.begin())
    {
        printf("Joybus emulation: no free PIO state machine or DMA channel\r\n");
    }
}

void JoybusEmulationDevice::end(bool full)
{
    m_controller.end();
}

void JoybusEmulationDevice::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    if (ConfigManager::instance().get_reinit_time() || ConfigManager::instance().is_reloading())
    {
        return;
    }
    bool comm = m_controller.is_communicating();
    if (comm != m_last_communicating)
    {
        m_last_communicating = comm;
        printf("Joybus emulation communication state changed: %d\r\n", comm);
        reload();
    }
}

bool JoybusEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.dataPin;
}
