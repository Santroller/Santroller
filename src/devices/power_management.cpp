#include "devices/power_management.hpp"
#include "managers/inactivity_manager.hpp"
#include "hardware/gpio.h"
#include "tusb.h"
#include "utils.h"

static void init_output(int32_t pin, bool invert)
{
    if (pin < 0)
    {
        return;
    }
    gpio_init(pin);
    gpio_put(pin, invert);
    gpio_set_dir(pin, GPIO_OUT);
}

PowerManagementDevice::PowerManagementDevice(proto_PowerManagementDevice device, uint16_t id) : Device(id), m_device(device)
{
    m_heartbeat_pin = device.has_heartbeatPin && device.has_heartbeatPeriodMs && device.heartbeatPeriodMs ? device.heartbeatPin : -1;
    m_inactivity_pin = device.has_inactivityPin && device.has_inactivityTimeoutMs && device.inactivityTimeoutMs ? device.inactivityPin : -1;
}

void PowerManagementDevice::begin()
{
    init_output(m_heartbeat_pin, m_device.heartbeatInvert);
    init_output(m_inactivity_pin, m_device.inactivityInvert);
}

void PowerManagementDevice::end(bool full)
{
    if (m_heartbeat_pin >= 0)
    {
        gpio_deinit(m_heartbeat_pin);
    }
    if (m_inactivity_pin >= 0)
    {
        gpio_deinit(m_inactivity_pin);
    }
}

void PowerManagementDevice::set_heartbeat(bool active)
{
    gpio_put(m_heartbeat_pin, active != m_device.heartbeatInvert);
}

void PowerManagementDevice::set_inactivity(bool active)
{
    gpio_put(m_inactivity_pin, active != m_device.inactivityInvert);
}

void PowerManagementDevice::update(bool full_poll, bool send_events)
{
    uint32_t now = millis();
    uint32_t idle = InactivityManager::instance().idle_ms(now);
    if (m_heartbeat_pin >= 0)
    {
        update_heartbeat(now, idle);
    }
    if (m_inactivity_pin >= 0)
    {
        update_inactivity(now, idle);
    }
}

void PowerManagementDevice::update_heartbeat(uint32_t now, uint32_t idle)
{
    // The heartbeat keeps its rhythm regardless of activity, but only pulses while in use
    if (!m_heartbeat_started || now - m_last_heartbeat >= m_device.heartbeatPeriodMs)
    {
        m_heartbeat_started = true;
        m_last_heartbeat = now;
        if (!m_device.heartbeatTimeoutMs || idle < m_device.heartbeatTimeoutMs)
        {
            set_heartbeat(true);
        }
    }
    if (now - m_last_heartbeat >= m_device.heartbeatPulseMs)
    {
        set_heartbeat(false);
    }
}

void PowerManagementDevice::update_inactivity(uint32_t now, uint32_t idle)
{
    if (m_device.inactivityNotOnUsb && tud_mounted())
    {
        m_pulsing = false;
        m_pulses_done = false;
        set_inactivity(false);
        return;
    }
    // Once started, the pulses always finish, as stopping halfway through a double press
    // could leave the module in the wrong state
    if (m_pulsing)
    {
        uint32_t period = m_device.inactivityPulsePeriodMs ? m_device.inactivityPulsePeriodMs : 1;
        uint32_t elapsed = now - m_pulses_start;
        if (elapsed >= period * m_device.inactivityPulseCount)
        {
            m_pulsing = false;
            m_pulses_done = true;
            set_inactivity(false);
            return;
        }
        set_inactivity(elapsed % period < m_device.inactivityPulseMs);
        return;
    }
    if (idle < m_device.inactivityTimeoutMs)
    {
        m_pulses_done = false;
        set_inactivity(false);
        return;
    }
    if (!m_device.inactivityPulseCount)
    {
        set_inactivity(true);
        return;
    }
    if (!m_pulses_done)
    {
        m_pulsing = true;
        m_pulses_start = now;
        set_inactivity(true);
    }
}

bool PowerManagementDevice::using_pin(uint8_t pin)
{
    return (m_heartbeat_pin >= 0 && pin == m_heartbeat_pin) || (m_inactivity_pin >= 0 && pin == m_inactivity_pin);
}
