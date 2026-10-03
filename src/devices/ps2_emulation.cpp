#include "devices/ps2_emulation.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
PSXEmulationDevice::PSXEmulationDevice(const DeviceReloadState *state, proto_PSXEmulationDevice device, uint16_t id)
    : Device(id), m_device(device), m_controller(device.clockPin, device.commandPin, device.dataPin, device.attentionPin, device.acknowledgePin)
{
    if (state && state->valid)
    {
        m_last_communicating = state->psx_emulation_communicating;
    }
}

void PSXEmulationDevice::save_reload_state(DeviceReloadState &state) const
{
    state.valid = true;
    state.psx_emulation_communicating = m_last_communicating;
}

void PSXEmulationDevice::begin()
{
    m_controller.begin(Gamepad);
}
void PSXEmulationDevice::end(bool full)
{
    m_controller.end();
}
void PSXEmulationDevice::rescan(bool first)
{
}
void PSXEmulationDevice::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    if (ConfigManager::instance().get_reinit_time() || ConfigManager::instance().is_reloading())
    {
        return;
    }
    bool comm = m_controller.is_communicating();
    if (comm != m_last_communicating)
    {
        m_last_communicating = comm;
        printf("PSX emulation communication state changed: %d\r\n", comm);
        reload();
    }
}

bool PSXEmulationDevice::using_pin(uint8_t pin)
{
    return pin == m_device.clockPin || pin == m_device.commandPin || pin == m_device.dataPin || pin == m_device.attentionPin || pin == m_device.acknowledgePin;
}
uint16_t PSXEmulationDevice::read_axis(proto_PS2AxisType type)
{
    return 0;
}
bool PSXEmulationDevice::read_button(proto_PS2ButtonType type)
{
    return false;
}