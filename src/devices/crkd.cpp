#include "devices/crkd.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
CrkdDevice::CrkdDevice(proto_CrkdNeckDevice device, uint16_t id) : Device(id), neck(device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate), m_device(device)
{
}

void CrkdDevice::update(bool full_poll, bool send_events)
{
    neck.tick();
    if (m_lastConnected != neck.is_connected() || full_poll)
    {
        m_lastConnected = neck.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool CrkdDevice::using_pin(uint8_t pin)
{
    return pin == m_device.uart.rx || pin == m_device.uart.tx;
}