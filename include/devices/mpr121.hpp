#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "libmpr121.hpp"
class MPR121Device : public Device
{
public:
    ~MPR121Device() {}
    MPR121Device(proto_Mpr121Device device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    // Electrodes 4 to 11 can be used as GPIO by inputs and LEDs instead of touch sensing
    // Touch sensing covers at least the configured electrode count and every touch input
    void use_touch(int32_t pin);
    void use_gpio_input(int32_t pin, PinMode pull);
    void use_gpio_output(int32_t pin);
    void set_output(int32_t pin, bool on);
    bool gpio_level(int32_t pin);
    MPR121 m_mpr121;

private:
    uint8_t touch_count() const;
    proto_Mpr121Device m_device;
    uint32_t m_last_value = 0;
    uint8_t m_touch_count = 0;
    uint8_t m_gpio_inputs = 0;
    uint8_t m_gpio_outputs = 0;
    uint8_t m_pull_ups = 0;
    uint8_t m_pull_downs = 0;
    uint8_t m_outputs = 0;
    bool m_config_dirty = true;
};
