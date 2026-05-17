#include "devices/protar_neck.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "usb/device/hid_device.h"
#include "config.hpp"
#include "utils.h"
#include "stdio.h"
#include <algorithm>
ProtarNeckDevice::ProtarNeckDevice(proto_ProtarNeckDevice device, uint16_t id) : Device(id), m_controller(device.spi.block, device.spi.sck, device.spi.mosi, device.spi.miso, device.spi.clock, device.attPin), m_device(device)
{
}

void ProtarNeckDevice::update(bool full_poll, bool send_events)
{
    m_controller.tick();
}

bool ProtarNeckDevice::using_pin(uint8_t pin)
{
    return pin == m_device.spi.mosi || pin == m_device.spi.miso || pin == m_device.spi.sck || pin == m_device.attPin;
}
uint16_t ProtarNeckDevice::readAxis(ProGuitarAxisType type)
{
    return m_controller.readAxis(type);
}
bool ProtarNeckDevice::readButton(ProGuitarButtonType type)
{
    return m_controller.readButton(type);
}