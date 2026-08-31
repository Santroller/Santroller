#pragma once
#include <stdint.h>

class LedMappingDevice
{
public:
    LedMappingDevice() {}
    virtual ~LedMappingDevice() {}
    virtual void set_val(uint16_t val) = 0;
    virtual void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) = 0;
    virtual void setup() = 0;
    virtual void off() = 0;
    virtual bool supports_brightness() = 0;
    virtual uint8_t led_count() = 0;
};
