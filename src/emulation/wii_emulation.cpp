#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <protocols/wii.hpp>
#include <pico/unique_id.h>
#include "emulation/wii_emulation.hpp"
#include "utils.h"

WiiEmulationDeviceInstance::WiiEmulationDeviceInstance(proto_WiiEmulationDevice device) : m_device(device), m_controller(device.i2c.block, device.i2c.sda, device.i2c.scl)
{
}
WiiEmulationDeviceInstance::~WiiEmulationDeviceInstance()
{
}
void WiiEmulationDeviceInstance::initialize()
{
    m_controller.begin(subtype);
    
}
void WiiEmulationDeviceInstance::process()
{
    // TODO: do we need to limit poll rate with this
    // m_device->ready();
    memcpy(m_buffer, initialReport, sizeof(initialReport));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
            mapping->update_wii(m_controller.wii_data_format(), m_buffer);
        }
        for (const auto &led : profile->leds)
        {
            led->update(false, false);
        }
    }
    // TODO: handle vibration and euphoria led
    m_controller.setInputs(m_buffer, m_size);
}