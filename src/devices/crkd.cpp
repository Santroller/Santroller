#include "devices/crkd.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
CrkdDevice::CrkdDevice(proto_CrkdNeckDevice device, uint16_t id) : Device(id), neck(device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate)
{
}

void CrkdDevice::update(bool full_poll)
{
    neck.tick();
    if (m_lastConnected != neck.isConnected() || full_poll || resend) {
        m_lastConnected = neck.isConnected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        resend = !HIDConfigDevice::send_event(event);
    }
}