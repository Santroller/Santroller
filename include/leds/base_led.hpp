#pragma once
#include <stdint.h>
#include "managers/inactivity_manager.hpp"

class LedMappingDevice
{
public:
    LedMappingDevice() {}
    virtual ~LedMappingDevice() {}
    // Writes are dropped while the LEDs are off for inactivity, so whatever drives them stays off
    void set_val(uint16_t val)
    {
        if (!InactivityManager::instance().leds_off())
        {
            write_val(val);
        }
    }
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
    {
        if (!InactivityManager::instance().leds_off())
        {
            write_val_raw(index, r, g, b, brightness);
        }
    }
    virtual void setup() = 0;
    virtual void off() = 0;
    virtual bool supports_brightness() = 0;
    virtual uint8_t led_count() = 0;

protected:
    virtual void write_val(uint16_t val) = 0;
    virtual void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) = 0;
};
