#include "devices/adc_battery.hpp"
#include "devices/pins.hpp"
#include "emulation/usb/hid_device.h"
#include "managers/battery_manager.hpp"
#include "utils.h"
#include <pico/cyw43_arch.h>

#define SAMPLE_COUNT 16

AdcBatteryDevice::AdcBatteryDevice(proto_AdcBatteryDevice device, uint16_t id) : Device(id), m_device(device)
{
    BatteryManager::instance().set_present();
    m_valid = is_adc_pin(device.pin) && device.fullMv > device.emptyMv;
}

void AdcBatteryDevice::begin()
{
}

void AdcBatteryDevice::end(bool full)
{
}

bool AdcBatteryDevice::read_mv(uint32_t &mv)
{
    // Scaled back up through the divider
    mv = adc_read_pin_mv(m_device.pin, SAMPLE_COUNT) * m_device.dividerX1000 / 1000;
    return true;
}

void AdcBatteryDevice::update(bool full_poll, bool send_events)
{
    uint32_t now = millis();
    if (m_valid && (!m_last_read || now - m_last_read >= 1000))
    {
        m_last_read = now;
        uint32_t mv;
        if (read_mv(mv))
        {
            m_mv = m_mv ? (m_mv * 3 + mv) / 4 : mv;
            int32_t level = 0;
            if (m_mv >= m_device.fullMv)
            {
                level = 100;
            }
            else if (m_mv > m_device.emptyMv)
            {
                level = (m_mv - m_device.emptyMv) * 100 / (m_device.fullMv - m_device.emptyMv);
            }
            BatteryManager::instance().set_level(level);
            if (level != m_last_level)
            {
                m_last_level = level;
                full_poll = true;
            }
        }
    }
    if (full_poll)
    {
        bool have = m_last_level >= 0;
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, have}}};
        event.event.device.has_battery = have;
        event.event.device.battery = have ? m_last_level : 0;
        HIDConfigDevice::send_event(event, true);
    }
}

bool AdcBatteryDevice::using_pin(uint8_t pin)
{
    return pin == m_device.pin;
}
