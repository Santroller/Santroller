#include "devices/bhdrum.hpp"
#include "events.pb.h"
#include "main.hpp"
BandHeroDrumDevice::BandHeroDrumDevice(proto_BandHeroDrumDevice device, uint16_t id) : Device(id), m_band_hero_drum(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock)
{
}

void BandHeroDrumDevice::update(bool resend_events) {
    m_band_hero_drum.tick();
    if (m_lastConnected != m_band_hero_drum.isConnected()) {
        m_lastConnected = m_band_hero_drum.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event, resend_events);
    }
}