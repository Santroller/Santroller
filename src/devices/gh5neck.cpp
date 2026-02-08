#include "devices/gh5neck.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
GH5NeckDevice::GH5NeckDevice(proto_GuitarHero5NeckDevice device, uint16_t id) : Device(id), m_gh5_neck(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}

void GH5NeckDevice::update(bool full_poll) {
    m_gh5_neck.tick();
    if (m_lastConnected != m_gh5_neck.is_connected() || full_poll || resend) {
        m_lastConnected = m_gh5_neck.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        resend = !HIDConfigDevice::send_event(event);
    }
}

bool GH5NeckDevice::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}