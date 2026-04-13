#include "devices/mpr121.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
MPR121Device::MPR121Device(proto_Mpr121Device device, uint16_t id) : Device(id), m_mpr121(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}

void MPR121Device::update(bool full_poll, bool send_events) {
    m_mpr121.tick();
    if (m_lastConnected != m_mpr121.is_connected() || full_poll) {
        m_lastConnected = m_mpr121.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool MPR121Device::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}