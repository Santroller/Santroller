#include "devices/mpr121.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
MPR121Device::MPR121Device(proto_Mpr121Device device, uint16_t id) : Device(id), m_mpr121(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void MPR121Device::update(bool full_poll) {
    m_mpr121.tick();
    if (m_lastConnected != m_mpr121.isConnected() || full_poll || resend) {
        m_lastConnected = m_mpr121.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        resend = !HIDConfigDevice::send_event(event);
    }
}