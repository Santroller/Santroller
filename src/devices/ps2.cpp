#include "devices/ps2.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
#include "utils.h"
PS2Device::PS2Device(proto_PSXDevice device, uint16_t id, MultitapPort port) : Device(id), m_controller(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin, device.ackPin, port), m_device(device), m_port(port)
{
}
void PS2Device::rescan(bool first)
{
    uint8_t current_seen_ports = 0;
    for (int i = 1; i <= 4; i++)
    {
        if (m_controller.controller_valid((MultitapPort)i))
        {
            current_seen_ports |= 1 << i;
            if (first)
            {
                auto &dev = assignable_devices.emplace_back(new PS2Device(m_device, m_id, (MultitapPort)i));
                dev->update(false, false);
            }
        }
    }
    if (current_seen_ports != m_last_seen_ports)
    {
        m_last_seen_ports = current_seen_ports;
        if (!first)
        {
            reload();
        }
    }
}
void PS2Device::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    if (m_port == BASE && (millis() - m_last_scan) > 500)
    {
        m_last_scan = millis();
        rescan(false);
        return;
    }
    if (m_controller.type != m_lastControllerType || full_poll || resend)
    {
        // don't trigger a reload when the controller is first identified
        if (m_controller.type != m_lastControllerType && m_lastControllerType != PS2ControllerTypeUnknown)
        {
            reload();
        }
        m_lastControllerType = m_controller.type;
        if (send_events)
        {
            proto_Event event = {which_event : proto_Event_ps2_tag, event : {ps2 : {m_id, m_lastControllerType}}};
            resend = !HIDConfigDevice::send_event(event);
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
bool PS2Device::isExtension(PS2ControllerType type)
{
    return m_controller.type == type;
}

bool PS2Device::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.miso || pin == m_device.spi.sck || pin == m_device.ackPin || pin == m_device.attPin;
}