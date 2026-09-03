#include "devices/protar_neck.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
ProtarNeckDevice::ProtarNeckDevice(proto_ProtarNeckDevice device, uint16_t id) : Device(id), m_controller(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin), m_device(device)
{
}
void ProtarNeckDevice::begin()
{
}
void ProtarNeckDevice::end(bool full)
{
}

void ProtarNeckDevice::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    if (m_lastConnected != m_controller.controller_valid() || full_poll) {
        m_lastConnected = m_controller.controller_valid();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool ProtarNeckDevice::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.miso || pin == m_device.spi.sck || pin == m_device.attPin;
}
uint16_t ProtarNeckDevice::read_axis(ProGuitarNeckAxisType type)
{
    return m_controller.read_axis(type);
}
bool ProtarNeckDevice::read_button(ProGuitarNeckButtonType type)
{
    return m_controller.read_button(type);
}