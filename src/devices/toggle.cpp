#include "devices/toggle.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
#include "utils.h"
ToggleDevice::ToggleDevice(proto_ToggleDevice device, uint16_t id, bool current) : Device(id), m_device(device),m_current_value(current)
{
}

void ToggleDevice::begin()
{
}
void ToggleDevice::end(bool full)
{
}
void ToggleDevice::update(bool full_poll, bool send_events)
{
}

void ToggleDevice::toggle()
{
    if (millis() - m_last < 100) {
        return;
    }
    m_last = millis();
    m_current_value = !m_current_value;
    proto_Event event = {which_event : proto_Event_toggle_tag, event : {toggle : {m_id, m_current_value}}};
    HIDConfigDevice::send_event(event, true);
    printf("toggle updated: %d %d\r\n", m_id, m_current_value);
    update_aux_toggle(m_id, m_current_value);
}

bool ToggleDevice::using_pin(uint8_t pin)
{
    return false;
}