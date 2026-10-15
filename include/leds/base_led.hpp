#pragma once
#include <stdint.h>
#include "managers/inactivity_manager.hpp"

// One physical LED: whatever drives it (a chip, or the Pico's own pins), and its position there
struct LedId
{
    const void *owner;
    uint32_t index;
    bool operator==(const LedId &other) const { return owner == other.owner && index == other.index; }
};

class LedMappingDevice
{
public:
    LedMappingDevice() {}
    virtual ~LedMappingDevice() {}
    // Writes are dropped while the LEDs are off for inactivity, so whatever drives them stays off
    void set_val(uint16_t val)
    {
        track(val);
        m_lit |= val != 0;
        if (!InactivityManager::instance().leds_off())
        {
            write_val(val);
        }
    }
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
    {
        track(index);
        track(r);
        track(g);
        track(b);
        track(brightness);
        m_lit |= r || g || b;
        if (!InactivityManager::instance().leds_off())
        {
            write_val_raw(index, r, g, b, brightness);
        }
    }
    virtual void setup() = 0;
    virtual void off() = 0;
    virtual void test_on() { set_val(UINT16_MAX); }
    virtual bool supports_brightness() = 0;
    virtual uint8_t led_count() = 0;
    // The physical LED the i'th LED of this device is
    virtual LedId led_id(uint8_t i) = 0;
    bool shares_led_with(LedMappingDevice &other)
    {
        for (uint8_t i = 0; i < led_count(); i++)
        {
            LedId id = led_id(i);
            for (uint8_t j = 0; j < other.led_count(); j++)
            {
                if (other.led_id(j) == id)
                {
                    return true;
                }
            }
        }
        return false;
    }

    // Everything written since start_tracking, folded into one value so a change can be spotted
    static constexpr uint32_t NOTHING_WRITTEN = 2166136261u;
    void start_tracking()
    {
        m_written = NOTHING_WRITTEN;
        m_lit = false;
    }
    uint32_t written() const { return m_written; }
    // Whether anything written since start_tracking would light an LED
    bool lit() const { return m_lit; }

protected:
    virtual void write_val(uint16_t val) = 0;
    virtual void write_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) = 0;

private:
    void track(uint32_t val)
    {
        m_written = (m_written ^ val) * 16777619u;
    }
    uint32_t m_written = NOTHING_WRITTEN;
    bool m_lit = false;
};
