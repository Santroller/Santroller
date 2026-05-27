#include "devices/vtechexpander.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "usb/device/hid_device.h"
VTechGuitarIOExpanderDevice::VTechGuitarIOExpanderDevice(proto_VTechGuitarIOExpanderDevice device, uint16_t id) : Device(id), m_vtech_expander(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin), m_device(device)
{
}
void VTechGuitarIOExpanderDevice::begin()
{
    m_vtech_expander.begin();
}

void VTechGuitarIOExpanderDevice::end(bool full)
{
    m_vtech_expander.end();
}
void VTechGuitarIOExpanderDevice::set_led(uint8_t i, uint8_t val) {
    m_vtech_expander.set_led(i, val);
}
void VTechGuitarIOExpanderDevice::update(bool full_poll, bool send_events)
{
    m_vtech_expander.tick();
    if (m_lastConnected != m_vtech_expander.is_connected() || full_poll)
    {
        m_lastConnected = m_vtech_expander.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}
bool VTechGuitarIOExpanderDevice::read_button(uint8_t pin) {
    return m_vtech_expander.read_button(pin);
}
bool VTechGuitarIOExpanderDevice::using_pin(uint8_t pin)
{
    return pin == m_device.spi.miso || pin == m_device.spi.mosi || pin == m_device.spi.sck || pin == m_device.attPin;
}