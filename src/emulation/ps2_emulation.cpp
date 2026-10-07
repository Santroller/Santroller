#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <protocols/ps2.hpp>
#include <pico/unique_id.h>
#include "emulation/ps2_emulation.hpp"
#include "devices/ps2_emulation.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"
const uint8_t guitar_dpad_bindings[] = {0x80, 0x23, 0xaa, 0x20, 0x58, 0x1c, 0x4b, 0x1a, 0xec, 0x23, 0xa1, 0x20, 0x55, 0x1b, 0x49, 0x1a};
Ps2EmulationDeviceInstance::Ps2EmulationDeviceInstance(proto_PSXEmulationDevice device)
    : m_device(device)
{
    auto psx_dev = DeviceManager::instance().get_psx_emulation_device();
    if (psx_dev)
    {
        m_psx_dev = psx_dev;
        m_controller = &psx_dev->get_controller();
    }
}
Ps2EmulationDeviceInstance::~Ps2EmulationDeviceInstance()
{
    if (!m_acquired)
    {
        return;
    }
    // Only release if the PSX device we acquired from still exists; if it was rewired or
    // removed, its controller is already gone and m_controller would be dangling.
    if (auto psx_dev = m_psx_dev.lock())
    {
        psx_dev->get_controller().release();
    }
}
void Ps2EmulationDeviceInstance::initialize()
{
    if (m_controller)
    {
        if (!m_acquired)
        {
            m_controller->acquire();
            m_acquired = true;
        }
        m_controller->begin(subtype);
    }
    switch (subtype)
    {
    case Gamepad:
    {
        PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)m_initial_report;
        memset(report, 0, sizeof(PS2Gamepad_Data_t));
        m_size = sizeof(PS2Gamepad_Data_t);
        report->leftStickX = PS3_STICK_CENTER;
        report->leftStickY = PS3_STICK_CENTER;
        report->rightStickX = PS3_STICK_CENTER;
        report->rightStickY = PS3_STICK_CENTER;

        break;
    }
    case GuitarHeroGuitar:
    {

        PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)m_initial_report;
        memset(report, 0, sizeof(PS2GuitarHeroGuitar_Data_t));
        m_size = sizeof(PS2GuitarHeroGuitar_Data_t);
        report->whammy = 0x7f;
        break;
    }
    // Digital only pads, the protocol layer only sends the first two bytes
    case Taiko:
    case PopNMusic:
    case BeatMania:
    {
        PS2Gamepad_Data_t *report = (PS2Gamepad_Data_t *)m_initial_report;
        memset(m_initial_report, 0, sizeof(m_initial_report));
        m_size = 2;
        if (subtype == PopNMusic)
        {
            // pop'n pads hold dpad left, right and down permanently as an identifier
            report->dpadLeft = 1;
            report->dpadRight = 1;
            report->dpadDown = 1;
        }
        break;
    }
    default:
        memset(m_initial_report, 0, sizeof(m_initial_report));
        m_size = 0;
        break;
    }
}

void Ps2EmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    memcpy(m_buffer, m_initial_report, sizeof(m_initial_report));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_ps2(m_buffer);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    // guitar hero guitars smashed the dpad into the right stick so we need to convert the bitmasks
    if (subtype == GuitarHeroGuitar) {
        PS2GuitarHeroGuitar_Data_t *report = (PS2GuitarHeroGuitar_Data_t *)m_buffer;
        report->dpad = guitar_dpad_bindings[report->dpad];
    }
    uint8_t small = 0, large = 0;
    if (m_controller)
    {
        m_controller->get_rumble(small, large);
    }
    set_rumble(large, small ? 255 : 0);

    if (!m_controller)
    {
        return;
    }

    if (subtype == Gamepad)
    {
        // Always hand the complete 18-byte controller report to the PSX
        // protocol layer. Formatting is deferred until the next SPI transaction
        // is armed, so the current command/state machine is authoritative.
        m_controller->getReportFormat();
        m_controller->sendData(18, m_buffer);
        return;
    }
    m_controller->sendData(m_size, m_buffer);
}
