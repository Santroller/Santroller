#include "devices/ps2.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
PS2Device::PS2Device(proto_PSXDevice device, uint16_t id, MultitapPort port) : Device(id), m_controller(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin, device.ackPin, port)
{
}

void PS2Device::update(bool full_poll)
{
    m_controller.tick();
    if (m_controller.type != m_lastControllerType || full_poll || resend)
    {
        if (m_controller.type != m_lastControllerType)
        {
            reload();
        }
        m_lastControllerType = m_controller.type;
        proto_Event event = {which_event : proto_Event_ps2_tag, event : {ps2 : {m_id, m_lastControllerType}}};
        resend = !HIDConfigDevice::send_event(event);
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
void PS2Device::load_devices()
{
    valid_devices.emplace_back(this);
    if (m_lastControllerType != PS2ControllerTypeUnknown)
    {
        assignable_devices.emplace_back(this);
    }
}