#include "devices/midiserial.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
MidiSerialDevice::MidiSerialDevice(proto_SerialMidiDevice device, uint16_t id) : MidiDevice(id, false), serial(this, device.uart.block, device.uart.tx, device.uart.rx, device.uart.baudrate), m_device(device)
{
}

void MidiSerialDevice::update(bool full_poll, bool send_events)
{
    serial.tick();
    MidiDevice::update(full_poll, send_events);
    if (send_events && (m_lastConnected != serial.is_connected() || full_poll))
    {
        m_lastConnected = serial.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event);
    }
}

bool MidiSerialDevice::using_pin(uint8_t pin)
{
    return pin == m_device.uart.rx || pin == m_device.uart.tx;
}