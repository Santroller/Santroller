#include "emulation/bt/wii_remote_emulation.hpp"
#include "emulation/wii_extension_input.hpp"
#include "protocols/wii.hpp"
#include "utils.h"
#include <string.h>

WiiRemoteEmulationDeviceInstance::WiiRemoteEmulationDeviceInstance()
{
}

WiiRemoteEmulationDeviceInstance::~WiiRemoteEmulationDeviceInstance()
{
    if (m_initialized)
    {
        wiimote_emulator_shutdown();
    }
}

void WiiRemoteEmulationDeviceInstance::initialize()
{
    m_report.mode = CLASSIC_CONTROLLER;
    m_report.extension_type = subtype == Gamepad ? WIIMOTE_EXTENSION_CLASSIC :
                              subtype == GuitarHeroGuitar ? WIIMOTE_EXTENSION_GUITAR :
                              subtype == GuitarHeroDrums ? WIIMOTE_EXTENSION_DRUMS :
                              subtype == DjHeroTurntable ? WIIMOTE_EXTENSION_TURNTABLE :
                              WIIMOTE_EXTENSION_CLASSIC;
    m_extension_format = wii_extension_format_for_subtype(subtype, 3);
    initialize_wii_extension_report(subtype, m_extension_format, m_extension_report, &m_extension_size,
                                    nullptr, nullptr);
    wiimote_emulator(&m_report);
    m_initialized = true;
}

void WiiRemoteEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    initialize_wii_extension_report(subtype, m_extension_format, m_extension_report, &m_extension_size,
                                    nullptr, nullptr);
    update_wii_extension_input(profiles, full_poll, send_events, m_extension_format, m_extension_report);
    m_report.extension_size = m_extension_size;
    memcpy(m_report.extension_data, m_extension_report, m_extension_size);

}
