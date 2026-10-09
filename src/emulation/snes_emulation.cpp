#include <string.h>
#include "emulation/snes_emulation.hpp"
#include "devices/snes_emulation.hpp"
#include "managers/device_manager.hpp"

SNESEmulationDeviceInstance::SNESEmulationDeviceInstance(proto_SNESEmulationDevice device)
    : m_device(device)
{
    auto snes_dev = DeviceManager::instance().get_snes_emulation_device();
    if (snes_dev)
    {
        m_snes_dev = snes_dev;
        m_controller = &snes_dev->get_controller();
        m_nes = m_controller->is_nes();
    }
}

SNESEmulationDeviceInstance::~SNESEmulationDeviceInstance()
{
    if (!m_acquired)
    {
        return;
    }
    // Only release if the device we acquired from still exists; if it was rewired or
    // removed, its controller is already gone and m_controller would be dangling.
    if (auto snes_dev = m_snes_dev.lock())
    {
        snes_dev->get_controller().release();
    }
}

void SNESEmulationDeviceInstance::initialize()
{
    if (m_controller && !m_acquired)
    {
        m_controller->acquire();
        m_acquired = true;
    }
}

void SNESEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    uint8_t buffer[sizeof(SNESGamepad_Data_t)] = {};
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            if (m_nes)
            {
                mapping->update_nes(buffer);
            }
            else
            {
                mapping->update_snes(buffer);
            }
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    if (m_controller)
    {
        m_controller->set_buttons(buffer[0] | (buffer[1] << 8));
    }
}
