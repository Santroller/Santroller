#include "devices/djh.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
DjHeroTurntableDevice::DjHeroTurntableDevice(proto_DJHeroTurntableDevice device, uint16_t id) :
    Device(id),
    m_turntable(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock, device.left,
                device.has_pollIntervalMs ? device.pollIntervalMs : DJH_DEFAULT_POLL_INTERVAL_MS),
    m_device(device)
{
}
void DjHeroTurntableDevice::begin()
{
    m_turntable.begin();
}

void DjHeroTurntableDevice::end(bool full)
{
    m_turntable.end();
}
void DjHeroTurntableDevice::update(bool full_poll, bool send_events) {
    m_turntable.tick();
    if (m_lastConnected != m_turntable.is_connected() || full_poll) {
        m_lastConnected = m_turntable.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool DjHeroTurntableDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}