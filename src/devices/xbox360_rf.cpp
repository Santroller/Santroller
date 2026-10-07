#include "devices/xbox360_rf.hpp"
#include "hardware/gpio.h"
#include "commands.pb.h"

Xbox360RfDevice::Xbox360RfDevice(proto_Xbox360RfDevice device, uint16_t id) : Device(id), m_device(device), m_rf(device.dataPin, device.clockPin, device.type == Xbox360RfSlim)
{
}

void Xbox360RfDevice::begin()
{
    if (m_device.has_syncPin && m_device.syncPin >= 0)
    {
        gpio_init(m_device.syncPin);
        gpio_set_dir(m_device.syncPin, GPIO_IN);
        gpio_pull_up(m_device.syncPin);
    }
    m_sync_pressed = false;
    m_rf.begin();
}

void Xbox360RfDevice::end(bool full)
{
    m_rf.end();
    if (m_device.has_syncPin && m_device.syncPin >= 0)
    {
        gpio_deinit(m_device.syncPin);
    }
}

void Xbox360RfDevice::update(bool full_poll, bool send_events)
{
    if (m_device.has_syncPin && m_device.syncPin >= 0)
    {
        bool pressed = !gpio_get(m_device.syncPin);
        if (pressed && !m_sync_pressed)
        {
            m_rf.send_sync();
        }
        m_sync_pressed = pressed;
    }
    m_rf.tick();
}

bool Xbox360RfDevice::using_pin(uint8_t pin)
{
    return pin == m_device.dataPin || pin == m_device.clockPin || (m_device.has_syncPin && pin == m_device.syncPin);
}

void Xbox360RfDevice::handle_command(proto_Command command)
{
    if (command.which_command == proto_Command_scan_tag)
    {
        m_rf.send_sync();
    }
}
