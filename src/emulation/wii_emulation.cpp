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
    initialize_wii_extension_report(subtype, format, initialReport, &m_size,
                                    &buttonsLowIdx, &buttonsHighIdx);
}
void WiiExtensionEmulationDeviceInstance::process(bool full_poll, bool send_events)
{
    // TODO: do we need to limit poll rate with this
    // m_device->ready();
    // if the format changed, reinitialize
    uint8_t format = wii_extension_format_for_subtype(subtype, m_controller.wii_data_format());
    if (lastFormat != format) {
        lastFormat = format;
        initialize();
    }
    memcpy(m_buffer, initialReport, sizeof(initialReport));
    update_wii_extension_input(profiles, full_poll, send_events,
                               m_controller.wii_data_format(), m_buffer);
    m_buffer[buttonsLowIdx] = ~m_buffer[buttonsLowIdx];
    m_buffer[buttonsHighIdx] = ~m_buffer[buttonsHighIdx];
    // TODO: euphoria led
    m_controller.setInputs(m_buffer, m_size);
}