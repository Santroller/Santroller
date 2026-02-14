#include "devices/crazyneck.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
CrazyGuitarNeckDevice::CrazyGuitarNeckDevice(proto_CrazyGuitarNeckDevice device, uint16_t id) : Device(id), m_crazy_guitar_neck(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}

void CrazyGuitarNeckDevice::update(bool full_poll, bool send_events)
{
    m_crazy_guitar_neck.tick();
    if (send_events && (m_lastConnected != m_crazy_guitar_neck.is_connected() || full_poll || resend))
    {
        m_lastConnected = m_crazy_guitar_neck.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        resend = !HIDConfigDevice::send_event(event);
    }
}

bool CrazyGuitarNeckDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}