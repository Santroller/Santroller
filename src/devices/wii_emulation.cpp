#include "devices/wii_emulation.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
#include <cstring>
WiiExtensionEmulationDevice::WiiExtensionEmulationDevice(const DeviceReloadState *state, proto_WiiEmulationDevice device, uint16_t id)
    : Device(id), m_device(device), m_controller(device.i2c.block, device.i2c.sda, device.i2c.scl)
{
    if (state && state->valid)
    {
        m_last_communicating = state->wii_emulation_communicating;
    }
}

bool WiiExtensionEmulationDevice::matches_reload_config(const proto_Device &config) const
{
    return config.which_device == proto_Device_wiiEmulation_tag &&
           memcmp(&m_device, &config.device.wiiEmulation, sizeof(m_device)) == 0;
}

void WiiExtensionEmulationDevice::save_reload_state(DeviceReloadState &state) const
{
    state.valid = true;
    state.wii_emulation_communicating = m_last_communicating;
}

void WiiExtensionEmulationDevice::begin()
{
    m_controller.begin(GuitarHeroGuitar);
}
void WiiExtensionEmulationDevice::end(bool full)
{
    m_controller.end();
}
void WiiExtensionEmulationDevice::rescan(bool first)
{
}
void WiiExtensionEmulationDevice::update(bool full_poll, bool send_events)
{
    m_controller.update();
    if (ConfigManager::instance().get_reinit_time() || ConfigManager::instance().is_reloading())
    {
        return;
    }
    bool comm = m_controller.is_communicating();
    if (comm != m_last_communicating)
    {
        m_last_communicating = comm;
        printf("Wii extension emulation communication state changed: %d\r\n", comm);
        reload();
    }
}

bool WiiExtensionEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.sda || pin == m_device.i2c.scl;
}
uint16_t WiiExtensionEmulationDevice::read_axis(proto_PS2AxisType type)
{
    return 0;
}
bool WiiExtensionEmulationDevice::read_button(proto_PS2ButtonType type)
{
    return false;
}