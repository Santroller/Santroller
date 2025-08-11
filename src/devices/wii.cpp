#include "devices/wii.hpp"
#include "events.pb.h"
#include "main.hpp"
WiiDevice::WiiDevice(proto_WiiDevice device, uint16_t id) : Device(id), m_extension(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void WiiDevice::update(bool resend_events) {
    m_extension.tick();
    if (m_extension.mType != m_lastExtType || resend_events) {

        m_lastExtType = m_extension.mType;
        proto_Event event = {which_event : proto_Event_wii_tag, event : {wii : {m_id, m_lastExtType}}};
        send_event(event, resend_events);
    }
    // if we have just picked up a device we should be emitting an event to the tool stating as such
}