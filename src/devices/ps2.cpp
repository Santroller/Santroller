#include "devices/ps2.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
#include "utils.h"
#include "stdio.h"
PS2Device::PS2Device(proto_PSXDevice device, uint16_t id, MultitapPort port) : Device(id), m_controller(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin, device.ackPin, port), m_device(device), m_port(port)
{
}
void PS2Device::rescan(bool first)
{
    if (first)
    {
        assignable_devices.push_back(active_devices.back());
        update(false, false);
        m_has_scanned = true;
    }
    else if (m_has_scanned && HIDConfigDevice::tool_closed())
    {
        reload();
    }
}
void PS2Device::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    if (m_controller.type != m_lastControllerType || full_poll)
    {
        bool changed = m_controller.type != m_lastControllerType;
        m_lastControllerType = m_controller.type;
        proto_Event event = {which_event : proto_Event_ps2_tag, event : {ps2 : {m_id, m_lastControllerType, port : m_port}}};
        HIDConfigDevice::send_event(event, true);
        if (changed)
        {
            rescan(false);
        }
    }
}
uint16_t PS2Device::readAxis(proto_PS2AxisType type)
{
    return m_controller.readAxis(type);
}
bool PS2Device::readButton(proto_PS2ButtonType type)
{
    return m_controller.readButton(type);
}
bool PS2Device::is_ps2_device(PS2ControllerType type)
{
    return m_lastControllerType == type;
}

bool PS2Device::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.miso || pin == m_device.spi.sck || pin == m_device.ackPin || pin == m_device.attPin;
}