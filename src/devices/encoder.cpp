#include "devices/encoder.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
#include "utils.h"
EncoderDevice::EncoderDevice(proto_EncoderDevice device, uint16_t id) : Device(id), encoder(device.dataPin), m_device(device), m_deltaRate(device.has_deltaRate ? device.deltaRate : 0)
{
}

void EncoderDevice::begin()
{
    encoder.begin();
}
void EncoderDevice::end(bool full)
{
    encoder.end();
}
void EncoderDevice::update(bool full_poll, bool send_events)
{
    if (m_deltaRate > 0 && (millis() - m_lastPoll) < m_deltaRate)
        return;
    m_lastPoll = millis();
    encoder.tick();
}

bool EncoderDevice::using_pin(uint8_t pin)
{
    return pin == m_device.dataPin || pin == (m_device.dataPin + 1);
}