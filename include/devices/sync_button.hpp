#pragma once
#include <stdint.h>
#include "hardware/gpio.h"

// An optional active low button that starts pairing / syncing a wireless controller
class SyncButton
{
public:
    explicit SyncButton(int32_t pin) : m_pin(pin) {}
    void begin()
    {
        if (m_pin < 0)
        {
            return;
        }
        gpio_init(m_pin);
        gpio_set_dir(m_pin, GPIO_IN);
        gpio_pull_up(m_pin);
        // a button held through boot / a reload shouldn't count as a press
        m_pressed = !gpio_get(m_pin);
    }
    void end()
    {
        if (m_pin >= 0)
        {
            gpio_deinit(m_pin);
        }
    }
    // true once per press
    bool pressed()
    {
        if (m_pin < 0)
        {
            return false;
        }
        bool pressed = !gpio_get(m_pin);
        bool edge = pressed && !m_pressed;
        m_pressed = pressed;
        return edge;
    }
    bool using_pin(uint8_t pin) const { return m_pin >= 0 && pin == m_pin; }

private:
    int32_t m_pin;
    bool m_pressed = false;
};
