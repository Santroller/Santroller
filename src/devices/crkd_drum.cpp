#include "devices/crkd_drum.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
CrkdDrumDevice::CrkdDrumDevice(proto_CrkdDrumDevice device, uint16_t id) : Device(id), drum(device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate), m_device(device)
{
}

void CrkdDrumDevice::begin()
{
    drum.begin();
}
void CrkdDrumDevice::end(bool full)
{
    drum.end();
}
void CrkdDrumDevice::update(bool full_poll, bool send_events)
{
    drum.tick();
    if (m_lastConnected != drum.is_connected() || full_poll)
    {
        m_lastConnected = drum.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool CrkdDrumDevice::using_pin(uint8_t pin)
{
    return pin == m_device.uart.rx || pin == m_device.uart.tx;
}