#include "devices/matrix.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
MatrixDevice::MatrixDevice(proto_MatrixDevice device, uint16_t id) : Device(id), m_matrix(device.inPins, device.outPins), m_device(device)
{
}

bool MatrixDevice::read_pin(uint8_t pin, uint32_t outPin)
{
    return m_matrix.read(outPin, pin);
}

void MatrixDevice::begin()
{
}
void MatrixDevice::end(bool full)
{
}
void MatrixDevice::update(bool full_poll, bool send_events)
{
}


bool MatrixDevice::using_pin(uint8_t pin)
{
    return (m_device.inPins | m_device.outPins) & (1 << pin);
}