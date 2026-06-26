#include "devices/wtdrum.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "usb/device/hid_device.h"
WorldTourDrumDevice::WorldTourDrumDevice(proto_WorldTourDrumDevice device, uint16_t id) : MidiDevice(id, false), m_world_tour_drum(this, device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.csPin), m_device(device)
{
}
void WorldTourDrumDevice::begin()
{
}

void WorldTourDrumDevice::end(bool full)
{
}
void WorldTourDrumDevice::update(bool full_poll, bool send_events)
{
    m_world_tour_drum.tick();
    if (m_lastConnected != m_world_tour_drum.is_connected() || full_poll)
    {
        m_lastConnected = m_world_tour_drum.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
    MidiDevice::update(full_poll, send_events);
}

bool WorldTourDrumDevice::using_pin(uint8_t pin)
{
    return pin == m_device.spi.miso ||pin == m_device.spi.mosi || pin == m_device.spi.sck || pin == m_device.csPin;
}