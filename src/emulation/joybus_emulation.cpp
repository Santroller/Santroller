#include <string.h>
#include "emulation/joybus_emulation.hpp"
#include "devices/joybus_emulation.hpp"
#include "managers/device_manager.hpp"

JoybusEmulationDeviceInstance::JoybusEmulationDeviceInstance(proto_JoybusEmulationDevice device)
    : m_device(device)
{
    auto joybus_dev = DeviceManager::instance().get_joybus_emulation_device();
    if (joybus_dev)
    {
        m_joybus_dev = joybus_dev;
        m_controller = &joybus_dev->get_controller();
        m_n64 = m_controller->is_n64();
    }
}

JoybusEmulationDeviceInstance::~JoybusEmulationDeviceInstance()
{
    if (!m_acquired)
    {
        return;
    }
    // Only release if the device we acquired from still exists; if it was rewired or
    // removed, its controller is already gone and m_controller would be dangling.
    if (auto joybus_dev = m_joybus_dev.lock())
    {
        joybus_dev->get_controller().release();
    }
}

void JoybusEmulationDeviceInstance::initialize()
{
    if (m_controller && !m_acquired)
    {
        m_controller->acquire();
        m_acquired = true;
    }
    memset(m_initial_report, 0, sizeof(m_initial_report));
    if (!m_n64)
    {
        GameCubeGamepad_Data_t *report = (GameCubeGamepad_Data_t *)m_initial_report;
        report->alwaysOne = 1;
        report->leftStickX = GC_STICK_CENTER;
        report->leftStickY = GC_STICK_CENTER;
        report->cStickX = GC_STICK_CENTER;
        report->cStickY = GC_STICK_CENTER;
    }
}

void JoybusEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    memcpy(m_buffer, m_initial_report, sizeof(m_initial_report));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            if (m_n64)
            {
                mapping->update_n64(m_buffer);
            }
            else
            {
                mapping->update_gamecube(m_buffer);
            }
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    if (!m_controller)
    {
        return;
    }
    // Only the GameCube controller has a rumble motor built in
    uint8_t rumble = m_controller->get_rumble() ? 255 : 0;
    set_rumble(rumble, rumble);
    m_controller->set_report(m_buffer);
}
