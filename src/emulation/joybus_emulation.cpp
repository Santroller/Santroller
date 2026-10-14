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
        m_n64 = joybus_dev->get_controller().is_n64();
    }
}

JoybusEmulationDeviceInstance::~JoybusEmulationDeviceInstance()
{
    if (!m_acquired)
    {
        return;
    }
    // Only release if the device we acquired from still exists; if it was rewired or
    // removed, its controller is already gone.
    if (auto joybus_dev = m_joybus_dev.lock())
    {
        joybus_dev->get_controller().release();
    }
}

void JoybusEmulationDeviceInstance::initialize()
{
    auto joybus_dev = m_joybus_dev.lock();
    if (joybus_dev && !m_acquired)
    {
        joybus_dev->get_controller().acquire();
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
    // The device can be rewired or removed while this instance still exists
    auto joybus_dev = m_joybus_dev.lock();
    if (!joybus_dev)
    {
        return;
    }
    JoybusEmulation &controller = joybus_dev->get_controller();
    // Only the GameCube controller has a rumble motor built in
    uint8_t rumble = controller.get_rumble() ? 255 : 0;
    set_rumble(rumble, rumble);
    controller.set_report(m_buffer);
}
