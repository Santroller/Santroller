#include "devices/snes_emulation.hpp"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include <cstring>
#include <stdio.h>

SNESEmulationDevice::SNESEmulationDevice(const DeviceReloadState *state, proto_SNESEmulationDevice device, uint16_t id)
    : Device(id), m_device(device), m_controller(device.clockPin, device.latchPin, device.dataPin, device.console == SNESConsoleNES)
{
    if (state && state->valid)
    {
        m_last_communicating = state->snes_emulation_communicating;
    }
}

bool SNESEmulationDevice::matches_reload_config(const proto_Device &config) const
{
    return config.which_device == proto_Device_snesEmulation_tag &&
           memcmp(&m_device, &config.device.snesEmulation, sizeof(m_device)) == 0;
}

void SNESEmulationDevice::save_reload_state(DeviceReloadState &state) const
{
    state.valid = true;
    state.snes_emulation_communicating = m_last_communicating;
}

void SNESEmulationDevice::begin()
{
    // Don't drive the data line until a profile is assigned, just watch latch for the console
    m_controller.begin();
}

void SNESEmulationDevice::end(bool full)
{
    m_controller.end();
}

void SNESEmulationDevice::update(bool full_poll, bool send_events)
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
        printf("SNES emulation communication state changed: %d\r\n", comm);
        reload();
    }
}

bool SNESEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.clockPin || pin == m_device.latchPin || pin == m_device.dataPin;
}
