#include "devices/gh5neck.hpp"
#include "events.pb.h"
#include "main.hpp"
GH5NeckDevice::GH5NeckDevice(proto_GuitarHero5NeckDevice device, uint16_t id) : Device(id), m_gh5_neck(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void GH5NeckDevice::update(bool full_poll) {
    m_gh5_neck.tick();
    if (m_lastConnected != m_gh5_neck.isConnected() || full_poll) {
        m_lastConnected = m_gh5_neck.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event);
    }
}