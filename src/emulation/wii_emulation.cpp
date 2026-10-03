#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <protocols/wii.hpp>
#include <pico/unique_id.h>
#include "emulation/wii_emulation.hpp"
#include "emulation/wii_extension_input.hpp"
#include "devices/wii_emulation.hpp"
#include "managers/device_manager.hpp"
#include "utils.h"

WiiExtensionEmulationDeviceInstance::WiiExtensionEmulationDeviceInstance(proto_WiiEmulationDevice device)
    : m_device(device)
{
    auto wii_dev = DeviceManager::instance().get_wii_emulation_device();
    if (wii_dev)
    {
        m_controller = &wii_dev->get_controller();
    }
}
WiiExtensionEmulationDeviceInstance::~WiiExtensionEmulationDeviceInstance()
{
}
void WiiExtensionEmulationDeviceInstance::initialize()
{
    if (!m_controller)
    {
        return;
    }
    m_controller->begin(subtype);
    uint8_t format = wii_extension_format_for_subtype(subtype, m_controller->wii_data_format());
    initialize_wii_extension_report(subtype, format, m_initial_report, &m_report_size,
                                    &m_buttons_low_idx, &m_buttons_high_idx);
}
void WiiExtensionEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    if (!m_controller)
    {
        return;
    }
    // if the format changed, reinitialize
    uint8_t format = wii_extension_format_for_subtype(subtype, m_controller->wii_data_format());
    if (m_last_format != format) {
        m_last_format = format;
        initialize();
    }
    memcpy(m_buffer, m_initial_report, sizeof(m_initial_report));
    update_wii_extension_input(profiles, full_poll, send_events,
                               m_controller->wii_data_format(), m_buffer);
    finalize_wii_extension_report(m_buffer, m_buttons_low_idx, m_buttons_high_idx);
    set_euphoria_led(m_controller->get_djh_euphoria_led_state() ? 255 : 0);
    m_controller->set_inputs(m_buffer, m_report_size);
}