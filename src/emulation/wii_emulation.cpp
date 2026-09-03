#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <protocols/wii.hpp>
#include <pico/unique_id.h>
#include "emulation/wii_emulation.hpp"
#include "emulation/wii_extension_input.hpp"
#include "utils.h"

WiiExtensionEmulationDeviceInstance::WiiExtensionEmulationDeviceInstance(proto_WiiEmulationDevice device) : m_device(device), m_controller(device.i2c.block, device.i2c.sda, device.i2c.scl)
{
}
WiiExtensionEmulationDeviceInstance::~WiiExtensionEmulationDeviceInstance()
{
}
void WiiExtensionEmulationDeviceInstance::initialize()
{
    m_controller.begin(subtype);
    uint8_t format = wii_extension_format_for_subtype(subtype, m_controller.wii_data_format());
    initialize_wii_extension_report(subtype, format, m_initial_report, &m_report_size,
                                    &m_buttons_low_idx, &m_buttons_high_idx);
}
void WiiExtensionEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    // if the format changed, reinitialize
    uint8_t format = wii_extension_format_for_subtype(subtype, m_controller.wii_data_format());
    if (m_last_format != format) {
        m_last_format = format;
        initialize();
    }
    memcpy(m_buffer, m_initial_report, sizeof(m_initial_report));
    update_wii_extension_input(profiles, full_poll, send_events,
                               m_controller.wii_data_format(), m_buffer);
    finalize_wii_extension_report(m_buffer, m_buttons_low_idx, m_buttons_high_idx);
    euphoria_led = m_controller.get_djh_euphoria_led_state();
    m_controller.set_inputs(m_buffer, m_report_size);
}