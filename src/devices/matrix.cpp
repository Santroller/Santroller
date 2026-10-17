#include "devices/matrix.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
static uint64_t combine_pins(uint32_t low, bool has_high, uint32_t high)
{
    return low | (has_high ? (uint64_t)high << 32 : 0);
}

MatrixDevice::MatrixDevice(proto_MatrixDevice device, uint16_t id)
    : Device(id),
      m_matrix(combine_pins(device.inPins, device.has_inPinsHigh, device.inPinsHigh), combine_pins(device.outPins, device.has_outPinsHigh, device.outPinsHigh)),
      m_in_pins(combine_pins(device.inPins, device.has_inPinsHigh, device.inPinsHigh)),
      m_out_pins(combine_pins(device.outPins, device.has_outPinsHigh, device.outPinsHigh))
{
}

bool MatrixDevice::read_pin(uint8_t pin, uint8_t out_pin)
{
    return m_matrix.read(out_pin, pin);
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
    return pin < 64 && ((m_in_pins | m_out_pins) & (1ull << pin));
}