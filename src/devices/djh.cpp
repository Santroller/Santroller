#include "devices/djh.hpp"
#include "events.pb.h"
#include "main.hpp"
DjHeroTurntableDevice::DjHeroTurntableDevice(proto_DJHeroTurntableDevice device, uint16_t id) : Device(id), m_turntable(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock, device.left)
{
}

void DjHeroTurntableDevice::update(bool resend_events) {
    m_turntable.tick();
    if (m_lastConnected != m_turntable.isConnected() || resend_events) {
        m_lastConnected = m_turntable.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event, resend_events);
    }
}