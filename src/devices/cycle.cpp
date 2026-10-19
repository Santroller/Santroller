#include "devices/cycle.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
CycleDevice::CycleDevice(const DeviceReloadState* state, proto_CycleDevice device, uint16_t id, uint32_t current_index, std::vector<uint32_t> states) : Device(id), m_device(device), m_states(states), m_current_value(0), m_current_index(current_index)
{
    // The value list can be emptied or shrunk after the index was saved
    if (m_current_index >= m_states.size())
    {
        m_current_index = 0;
    }
    if (!m_states.empty())
    {
        m_current_value = m_states[m_current_index];
    }
    if (state) {
        m_last = state->debounce_time;
        m_forward_held = state->cycle_forward_held;
        m_backward_held = state->cycle_backward_held;
    }
}

void CycleDevice::save_reload_state(DeviceReloadState& state) const
{
    state.valid = true;
    state.debounce_time = m_last;
    state.cycle_forward_held = m_forward_held;
    state.cycle_backward_held = m_backward_held;
}

void CycleDevice::begin()
{
    m_last = millis();
}
void CycleDevice::end(bool full)
{
}
void CycleDevice::update(bool full_poll, bool send_events)
{
}

void CycleDevice::cycle(bool forward)
{
    if (m_states.empty() || millis() - m_last < 100) {
        return;
    }
    m_last = millis();
    set_index(step_index(forward));
}

uint32_t CycleDevice::step_index(bool forward) const
{
    if (forward)
    {
        return m_current_index + 1 >= m_states.size() ? 0 : m_current_index + 1;
    }
    return m_current_index == 0 ? m_states.size() - 1 : m_current_index - 1;
}

void CycleDevice::set_index(uint32_t index)
{
    m_current_index = index;
    proto_Event event = {which_event : proto_Event_cycle_tag, event : {cycle : {m_id, m_current_index}}};
    HIDConfigDevice::send_event(event, true);
    m_current_value = m_states[m_current_index];
    printf("cycle updated: %d %d\r\n", m_current_index, m_current_value);
    update_aux_cycle(m_id, m_current_index);
}

// From the host, so no debounce: that is only there for the physical inputs
void CycleDevice::handle_command(proto_Command command)
{
    if (command.which_command != proto_Command_setCycle_tag || m_states.empty())
    {
        return;
    }
    const auto &cmd = command.command.setCycle;
    if (cmd.has_state)
    {
        if (cmd.state < m_states.size() && cmd.state != m_current_index)
        {
            set_index(cmd.state);
        }
        return;
    }
    set_index(step_index(!cmd.has_forward || cmd.forward));
}

bool CycleDevice::using_pin(uint8_t pin)
{
    return false;
}