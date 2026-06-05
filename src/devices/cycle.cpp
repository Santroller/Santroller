#include "devices/cycle.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
CycleDevice::CycleDevice(proto_CycleDevice device, uint16_t id, uint32_t current_index, std::vector<uint32_t> states) : Device(id), m_device(device), m_states(states), m_current_value(states[current_index]), m_current_index(current_index)
{

}

void CycleDevice::begin()
{
}
void CycleDevice::end(bool full)
{
}
void CycleDevice::update(bool full_poll, bool send_events)
{
}

void CycleDevice::cycle()
{
    m_current_index++;
    if (m_current_index >= m_states.size())
    {
        m_current_index = 0;
    }
    proto_Event event = {which_event : proto_Event_cycle_tag, event : {cycle : {m_id, m_current_index}}};
    HIDConfigDevice::send_event(event, true);
    m_current_value = m_states[m_current_index];
    update_aux(m_id, m_current_index);
}

bool CycleDevice::using_pin(uint8_t pin)
{
    return false;
}