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
    gpio_pull_up_mask(m_device.pins);
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

    // One side of the switch is driven low; every other network node is
    // left as a pulled-up input. A closed switch therefore pulls other_pin
    // low. Returning all pins to inputs prevents the network from being
    // actively driven between scans.
    gpio_set_dir_in_masked(m_device.pins);
    gpio_pull_up_mask(m_device.pins);

    gpio_set_dir_out_masked(pin_mask);
    gpio_put_masked(pin_mask, 0);

    sleep_us(1);
    const bool pressed = gpio_get(other_pin) == 0;

    gpio_set_dir_in_masked(pin_mask | other_mask);
    gpio_pull_up_mask(pin_mask | other_mask);

    return pressed;
}
