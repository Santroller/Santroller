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
    for (auto &report : m_report_buffers)
    {
        report.mode = CLASSIC_CONTROLLER;
        report.extension_format = 1;
        report.extension_type = subtype == Gamepad ? WIIMOTE_EXTENSION_CLASSIC :
                                  subtype == GuitarHeroGuitar ? WIIMOTE_EXTENSION_GUITAR :
                                  subtype == GuitarHeroDrums ? WIIMOTE_EXTENSION_DRUMS :
                                  subtype == DjHeroTurntable ? WIIMOTE_EXTENSION_TURNTABLE :
                                  WIIMOTE_EXTENSION_CLASSIC;
    }
    m_extension_format = wii_extension_format_for_subtype(subtype, 1);
    initialize_wii_extension_report(subtype, m_extension_format, m_extension_initial_report, &m_extension_size,
                                    &m_buttons_low_idx, &m_buttons_high_idx);
    memcpy(m_extension_report, m_extension_initial_report, sizeof(m_extension_report));
    m_active_buffer = 0;
    BluetoothStack::instance().request_wiimote(&m_report_buffers[m_active_buffer]);
    m_initialized = true;
}

void WiiRemoteEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    // The active buffer is the one currently published to the BT stack; it may still
    // be read from the CAN_SEND_NOW IRQ, so it must not be mutated here. The new frame
    // is built entirely in the other (inactive) buffer and published with a single
    // atomic pointer swap, so the IRQ never observes a partially-updated report.
    WiimoteReport &current = m_report_buffers[m_active_buffer];
    uint8_t next_index = 1 - m_active_buffer;
    WiimoteReport &next = m_report_buffers[next_index];

    uint8_t extension_format = wii_extension_format_for_subtype(subtype, current.extension_format);
    if (m_extension_format != extension_format)
    {
        m_extension_format = extension_format;
        initialize_wii_extension_report(subtype, m_extension_format, m_extension_initial_report, &m_extension_size,
                                        &m_buttons_low_idx, &m_buttons_high_idx);
    }
    euphoria_led = current.euphoria_led;
    player_led = current.player_led;
    rumble_left = current.rumble ? 0xff : 0;
    rumble_right = rumble_left;

    // Carry forward the fields the BT stack owns/writes, so they aren't lost on swap.
    next.mode = current.mode;
    next.extension_type = current.extension_type;
    next.extension_format = current.extension_format;
    next.euphoria_led = current.euphoria_led;
    next.player_led = current.player_led;
    next.rumble = current.rumble;
    next.reset_ir = current.reset_ir;
    next.fake_motion = current.fake_motion;
    next.center_accel = current.center_accel;

    memset(&next.wiimote, 0, sizeof(next.wiimote));
    memcpy(m_extension_report, m_extension_initial_report, sizeof(m_extension_report));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            // mapping->update_wiimote_core(&next.wiimote);
            mapping->update_wii(m_extension_format, m_extension_report);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    finalize_wii_extension_report(m_extension_report, m_buttons_low_idx, m_buttons_high_idx);
    next.extension_size = m_extension_size;
    memcpy(next.extension_data, m_extension_report, m_extension_size);

    m_active_buffer = next_index;
    BluetoothStack::instance().update_wiimote_report(&next);
}
