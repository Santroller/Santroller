#include "emulation/bt/wii_remote_emulation.hpp"
#include "emulation/wii_extension_input.hpp"
#include "protocols/wii.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "utils.h"
#include <string.h>

WiiRemoteEmulationDeviceInstance::WiiRemoteEmulationDeviceInstance()
{
}

WiiRemoteEmulationDeviceInstance::~WiiRemoteEmulationDeviceInstance()
{
    if (m_initialized)
    {
        BluetoothStack::instance().release_wiimote();
    }
}

void WiiRemoteEmulationDeviceInstance::initialize()
{
    m_report.mode = CLASSIC_CONTROLLER;
    m_report.extension_format = 1;
    m_report.extension_type = subtype == Gamepad ? WIIMOTE_EXTENSION_CLASSIC :
                              subtype == GuitarHeroGuitar ? WIIMOTE_EXTENSION_GUITAR :
                              subtype == GuitarHeroDrums ? WIIMOTE_EXTENSION_DRUMS :
                              subtype == DjHeroTurntable ? WIIMOTE_EXTENSION_TURNTABLE :
                              WIIMOTE_EXTENSION_CLASSIC;
    m_extension_format = wii_extension_format_for_subtype(subtype, 1);
    initialize_wii_extension_report(subtype, m_extension_format, m_extension_initial_report, &m_extension_size,
                                    &m_buttons_low_idx, &m_buttons_high_idx);
    memcpy(m_extension_report, m_extension_initial_report, sizeof(m_extension_report));
    BluetoothStack::instance().request_wiimote(&m_report);
    m_initialized = true;
}

void WiiRemoteEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    uint8_t extension_format = wii_extension_format_for_subtype(subtype, m_report.extension_format);
    if (m_extension_format != extension_format)
    {
        m_extension_format = extension_format;
        initialize_wii_extension_report(subtype, m_extension_format, m_extension_initial_report, &m_extension_size,
                                        &m_buttons_low_idx, &m_buttons_high_idx);
    }
    euphoria_led = m_report.euphoria_led;
    player_led = m_report.player_led;
    rumble_left = m_report.rumble ? 0xff : 0;
    rumble_right = rumble_left;
    memset(&m_report.wiimote, 0, sizeof(m_report.wiimote));
    memcpy(m_extension_report, m_extension_initial_report, sizeof(m_extension_report));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            // mapping->update_wiimote_core(&m_report.wiimote);
            mapping->update_wii(m_extension_format, m_extension_report);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    finalize_wii_extension_report(m_extension_report, m_buttons_low_idx, m_buttons_high_idx);
    m_report.extension_size = m_extension_size;
    memcpy(m_report.extension_data, m_extension_report, m_extension_size);

}
