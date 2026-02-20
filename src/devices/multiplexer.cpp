#include "devices/multiplexer.hpp"
#include "events.pb.h"
#include "config.hpp"
#include "main.hpp"
#include "usb/device/hid_device.h"
MultiplexerDevice::MultiplexerDevice(proto_MultiplexerDevice device, uint16_t id) : Device(id), m_device(device), m_multiplexer(device.s0Pin, device.s1Pin, device.s2Pin, device.s3Pin, device.inputPin, device.sixteenChannel)
{
}

void MultiplexerDevice::update(bool full_poll, bool send_events)
{
}

uint16_t MultiplexerDevice::read(uint8_t channel) {
    return m_multiplexer.read(channel);
}

bool MultiplexerDevice::using_pin(uint8_t pin)
{
    return (m_device.sixteenChannel && pin == m_device.s3Pin) || pin == m_device.s0Pin || pin == m_device.s1Pin || pin == m_device.s2Pin || pin == m_device.inputPin;
}