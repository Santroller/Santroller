#include "devices/adc_battery.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "emulation/usb/hid_device.h"
#include "managers/battery_manager.hpp"
#include "hardware/adc.h"
#include "utils.h"
#include <pico/cyw43_arch.h>

#define ADC_FIRST_PIN 26
#define ADC_PIN_COUNT 4
#define SAMPLE_COUNT 16

AdcBatteryDevice::AdcBatteryDevice(proto_AdcBatteryDevice device, uint16_t id) : Device(id), m_device(device)
{
    BatteryManager::instance().set_present();
    m_valid = device.pin >= ADC_FIRST_PIN && device.pin < ADC_FIRST_PIN + ADC_PIN_COUNT && device.fullMv > device.emptyMv;
}

void AdcBatteryDevice::begin()
{
}

void AdcBatteryDevice::end(bool full)
{
}

bool AdcBatteryDevice::read_mv(uint32_t &mv)
{
    uint32_t total = 0;
    {
        // On a Pico W, GP29 is also the wireless chip's SPI clock. Holding the stack lock keeps
        // the chip off the bus while it is used as an ADC, and the driver takes the pin back at
        // the start of its next transfer. Without bluetooth this lock does nothing.
        BtStackLock lock;
        adc_gpio_init(m_device.pin);
        adc_select_input(m_device.pin - ADC_FIRST_PIN);
        // the first readings after switching inputs can be low
        adc_read();
        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            total += adc_read();
        }
    }
    uint32_t raw = total / SAMPLE_COUNT;
    // 12 bit reading of 0 to 3.3V, scaled back up through the divider
    mv = raw * 3300 / 4095 * m_device.dividerX1000 / 1000;
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
