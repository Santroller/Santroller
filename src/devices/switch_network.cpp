#include "devices/switch_network.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"

SwitchNetworkDevice::SwitchNetworkDevice(proto_SwitchNetworkDevice device, uint16_t id)
    : Device(id), m_device(device)
{
}

void SwitchNetworkDevice::begin()
{
    gpio_init_mask(m_device.pins);
    gpio_set_dir_in_masked(m_device.pins);
    for (uint8_t pin = 0; pin < 32; ++pin)
    {
        if (using_pin(pin))
            gpio_pull_up(pin);
    }
}

void SwitchNetworkDevice::end(bool full)
{
    (void)full;
    gpio_set_dir_in_masked(m_device.pins);
}

void SwitchNetworkDevice::update(bool full_poll, bool send_events)
{
    (void)full_poll;
    (void)send_events;
}

bool SwitchNetworkDevice::using_pin(uint8_t pin)
{
    return (m_device.pins & (1u << pin)) != 0;
}

bool SwitchNetworkDevice::read_switch(uint8_t pin, uint8_t other_pin)
{
    const uint32_t pin_mask = 1u << pin;
    const uint32_t other_mask = 1u << other_pin;

    if (!using_pin(pin) || !using_pin(other_pin) || pin == other_pin)
        return false;

    gpio_set_dir_in_masked(m_device.pins);
    for (uint8_t pin = 0; pin < 32; ++pin)
    {
        if (using_pin(pin))
            gpio_pull_up(pin);
    }

    gpio_set_dir_out_masked(pin_mask);
    gpio_put_masked(pin_mask, 0);

    sleep_us(1);
    const bool pressed = gpio_get(other_pin) == 0;

    gpio_set_dir_in_masked(pin_mask | other_mask);
    for (uint8_t pin = 0; pin < 32; ++pin)
    {
        if ((pin_mask | other_mask) & (1u << pin))
            gpio_pull_up(pin);
    }

    return pressed;
}

bool SwitchNetworkDevice::read_button(uint8_t button)
{
    if (button >= m_device.buttons_count)
        return false;

    const auto &button_config = m_device.buttons[button];
    return read_switch(
        static_cast<uint8_t>(button_config.pin),
        static_cast<uint8_t>(button_config.otherPin));
}
