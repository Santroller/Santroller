#include "devices/xbox360_rf.hpp"
#include "commands.pb.h"

Xbox360RfDevice::Xbox360RfDevice(proto_Xbox360RfDevice device, uint16_t id) : Device(id), m_device(device), m_rf(device.dataPin, device.clockPin, device.type == Xbox360RfSlim), m_sync(device.has_syncPin ? device.syncPin : -1)
{
}

void Xbox360RfDevice::begin()
{
    m_sync.begin();
    m_rf.begin();
}

void Xbox360RfDevice::end(bool full)
{
    m_rf.end();
    m_sync.end();
}

void Xbox360RfDevice::update(bool full_poll, bool send_events)
{
    if (m_sync.pressed())
    {
        m_rf.send_sync();
    }
    m_rf.tick();
}

bool Xbox360RfDevice::using_pin(uint8_t pin)
{
    return pin == m_device.dataPin || pin == m_device.clockPin || m_sync.using_pin(pin);
}

void Xbox360RfDevice::handle_command(proto_Command command)
{
    if (command.which_command == proto_Command_scan_tag)
    {
        m_rf.send_sync();
    }
}
