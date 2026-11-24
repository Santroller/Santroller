#pragma once
#include "base.hpp"
#include "device.pb.h"

// Plain LEDs on the Pico's own pins, grouped so one LED mapping can drive several of them. The group only
// holds their pins: the mappings using it set the pins up and drive them, as they do a single GPIO LED.
class GpioLedGroupDevice : public Device
{
public:
    GpioLedGroupDevice(proto_GpioLedGroupDevice device, uint16_t id) : Device(id), m_device(device) {}
    void begin() {}
    void end(bool full) {}
    void update(bool full_poll, bool send_events) {}
    bool using_pin(uint8_t pin)
    {
        for (pb_size_t i = 0; i < m_device.pins_count; i++)
        {
            if (m_device.pins[i] == pin)
            {
                return true;
            }
        }
        return false;
    }
    uint8_t pin_count() const { return m_device.pins_count; }
    // The i'th pin as a single GPIO LED, -1 past the end
    proto_GpioLedDevice pin(uint8_t i) const
    {
        proto_GpioLedDevice led = proto_GpioLedDevice_init_zero;
        led.pin = i < m_device.pins_count ? m_device.pins[i] : -1;
        led.analog = m_device.analog;
        led.has_inverted = m_device.has_inverted;
        led.inverted = m_device.inverted;
        return led;
    }

private:
    proto_GpioLedGroupDevice m_device;
};
