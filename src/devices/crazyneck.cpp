#include "devices/crazyneck.hpp"
#include "events.pb.h"
#include "main.hpp"
CrazyGuitarNeckDevice::CrazyGuitarNeckDevice(proto_CrazyGuitarNeckDevice device, uint16_t id) : Device(id), m_crazy_guitar_neck(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void CrazyGuitarNeckDevice::update(bool full_poll) {
    m_crazy_guitar_neck.tick();
    if (m_lastConnected != m_crazy_guitar_neck.isConnected() || full_poll) {
        m_lastConnected = m_crazy_guitar_neck.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event);
    }
}