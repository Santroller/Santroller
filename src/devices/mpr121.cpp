#include "devices/mpr121.hpp"
#include "events.pb.h"
#include "main.hpp"
#include "emulation/usb/hid_device.h"
#include "config/config.hpp"
MPR121Device::MPR121Device(proto_Mpr121Device device, uint16_t id) : Device(id), m_mpr121(device.i2c.block, device.i2c.sda, device.i2c.scl, device.i2c.clock), m_device(device)
{
}

uint8_t MPR121Device::touch_count() const
{
    uint8_t configured = m_device.touchpadCount > 0 ? m_device.touchpadCount : 0;
    return configured > m_touch_count ? configured : m_touch_count;
}

static int8_t gpio_bit(int32_t pin)
{
    return pin >= MPR121_FIRST_GPIO && pin < MPR121_ELECTRODES ? pin - MPR121_FIRST_GPIO : -1;
}

void MPR121Device::use_touch(int32_t pin)
{
    if (pin >= 0 && pin < MPR121_ELECTRODES && pin + 1 > m_touch_count)
    {
        m_touch_count = pin + 1;
        m_config_dirty = true;
    }
}

void MPR121Device::use_gpio_input(int32_t pin, PinMode pull)
{
    int8_t bit = gpio_bit(pin);
    if (bit < 0)
    {
        return;
    }
    m_gpio_inputs |= 1 << bit;
    if (pull == PinMode_PullUp)
    {
        m_pull_ups |= 1 << bit;
    }
    else if (pull == PinMode_PullDown)
    {
        m_pull_downs |= 1 << bit;
    }
    m_config_dirty = true;
}

void MPR121Device::use_gpio_output(int32_t pin)
{
    int8_t bit = gpio_bit(pin);
    if (bit < 0)
    {
        return;
    }
    m_gpio_outputs |= 1 << bit;
    m_config_dirty = true;
}

void MPR121Device::set_output(int32_t pin, bool on)
{
    int8_t bit = gpio_bit(pin);
    if (bit < 0)
    {
        return;
    }
    if (on)
    {
        m_outputs |= 1 << bit;
    }
    else
    {
        m_outputs &= ~(1 << bit);
    }
}

bool MPR121Device::gpio_level(int32_t pin)
{
    int8_t bit = gpio_bit(pin);
    return bit >= 0 && (m_mpr121.gpio & (1 << bit));
}

void MPR121Device::begin()
{
    m_mpr121.configure(touch_count(), m_gpio_inputs, m_gpio_outputs, m_pull_ups, m_pull_downs);
    m_config_dirty = false;
    m_mpr121.begin();
}
void MPR121Device::end(bool full)
{
    m_mpr121.end();
}
void MPR121Device::update(bool full_poll, bool send_events) {
    // Inputs and LEDs claim their pins after the device has started
    if (m_config_dirty)
    {
        m_config_dirty = false;
        m_mpr121.configure(touch_count(), m_gpio_inputs, m_gpio_outputs, m_pull_ups, m_pull_downs);
    }
    m_mpr121.set_outputs(m_outputs);
    m_mpr121.tick();
    if (m_lastConnected != m_mpr121.is_connected() || full_poll) {
        m_lastConnected = m_mpr121.is_connected();
        proto_Event event = {which_event : proto_Event_device_tag, event : {device : {m_id, m_lastConnected}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool MPR121Device::using_pin(uint8_t pin)
{
    return pin == m_device.i2c.scl || pin == m_device.i2c.sda;
}