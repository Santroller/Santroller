#include "devices/mpr121.hpp"
#include "events.pb.h"
#include "main.hpp"
MPR121Device::MPR121Device(proto_Mpr121Device device, uint16_t id) : Device(id), m_mpr121(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void MPR121Device::update(bool resend_events) {
    m_mpr121.tick();
    if (m_lastConnected != m_mpr121.isConnected() || resend_events) {
        m_lastConnected = m_mpr121.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event, resend_events);
    }
}