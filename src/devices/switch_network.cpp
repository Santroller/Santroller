#include "devices/switch_network.hpp"
#include "hardware/gpio.h"
#include "pico/time.h"

SwitchNetworkDevice::SwitchNetworkDevice(proto_SwitchNetworkDevice device, uint16_t id)
    : Device(id), m_device(device)
{
}

void SwitchNetworkDevice::idle_pins()
{
    for (uint8_t pin = 0; pin < NUM_BANK0_GPIOS; ++pin)
    {
        if (using_pin(pin))
        {
            gpio_set_dir(pin, GPIO_IN);
            gpio_pull_up(pin);
        }
    }
}

void SwitchNetworkDevice::begin()
{
    for (uint8_t pin = 0; pin < NUM_BANK0_GPIOS; ++pin)
    {
        if (using_pin(pin))
        {
            gpio_init(pin);
        }
    }
    idle_pins();
}

void SwitchNetworkDevice::end(bool full)
{
    (void)full;
    for (uint8_t pin = 0; pin < NUM_BANK0_GPIOS; ++pin)
    {
        if (using_pin(pin))
        {
            gpio_set_dir(pin, GPIO_IN);
        }
    }
}

void SwitchNetworkDevice::update(bool full_poll, bool send_events)
{
    (void)full_poll;
    (void)send_events;
}

bool SwitchNetworkDevice::using_pin(uint8_t pin)
{
    return pin < 64 && (pin_mask() & (1ull << pin)) != 0;
}

bool SwitchNetworkDevice::read_switch(uint8_t pin, uint8_t other_pin)
{
    if (!using_pin(pin) || !using_pin(other_pin) || pin == other_pin)
        return false;

    // Everything else floats, so only this pair is in the circuit
    for (uint8_t other = 0; other < NUM_BANK0_GPIOS; ++other)
    {
        if (using_pin(other))
        {
            gpio_set_dir(other, GPIO_IN);
            gpio_set_pulls(other, false, false);
        }
    }

    gpio_put(pin, 0);
    gpio_pull_up(other_pin);
    gpio_set_dir(pin, GPIO_OUT);

    sleep_us(1);
    const bool pressed = gpio_get(other_pin) == 0;

    idle_pins();
    return pressed;
}

bool SwitchNetworkDevice::read_button(uint8_t button)
{
    if (button >= m_device.buttons_count)
        return false;

    const auto &button_config = m_device.buttons[button];
    return read_switch(button_config.pin, button_config.otherPin);
}
