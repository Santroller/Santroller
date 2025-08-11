#include "devices/max1704x.hpp"
#include "events.pb.h"
#include "main.hpp"
Max1704XDevice::Max1704XDevice(proto_Max1704xDevice device, uint16_t id) : Device(id), m_max1704x(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void Max1704XDevice::update(bool resend_events) {
    m_max1704x.tick();
    if (m_lastConnected != m_max1704x.isConnected() || resend_events) {
        m_lastConnected = m_max1704x.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event, resend_events);
    }
}