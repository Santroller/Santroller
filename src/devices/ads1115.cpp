#include "devices/ads1115.hpp"
#include "events.pb.h"
#include "main.hpp"
ADS1115Device::ADS1115Device(proto_ADS1115Device device, uint16_t id) : Device(id), ads1115(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock, device.interrupt)
{
}

void ADS1115Device::update(bool full_poll)
{
    ads1115.tick();
    if (m_lastConnected != ads1115.isConnected() || full_poll) {
        m_lastConnected = ads1115.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        send_event(event);
    }
}