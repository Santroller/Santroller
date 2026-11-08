#pragma once
#include <stdint.h>
#include <vector>
#include "enums.pb.h"
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
    void set_val(uint16_t val);
    // Drives only the index'th LED of this device, as set_val drives all of them
    void set_val_at(uint8_t index, uint16_t val);
    void set_val_raw(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    // Draws the effect over everything written from now on, while it lights the LED
    void set_effect(LedEffectType type, uint32_t speed);
    bool has_effect() const { return m_effect_type != 0; }
    // Draws the lit LEDs again with the effect's current frame, as a mapping the host drives only writes
    // them when the host changes something
    void redraw_effect();
    // Forgets what was written, so the effect doesn't light it again
    void forget_written();
    virtual void setup() = 0;
    virtual void off() = 0;
    // Tests show the LED as it is, without the effect
    virtual void test_on() { write_val(UINT16_MAX); }
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
    // Devices with a single LED have nothing else to drive
    virtual void write_val_at(uint8_t index, uint16_t val) { write_val(val); }
    // RGB devices: the colour val shows. false when that's black with no released colour, which leaves the
    // LED to whatever else drives it. Other devices have no colour, so the effect dims val instead.
    virtual bool is_rgb() { return false; }
    virtual bool colour(uint16_t val, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w) { return false; }

private:
    // What the mapping last asked one LED to show, which the effect draws over
    struct Written
    {
        uint16_t val;
        uint8_t r, g, b, brightness;
        bool raw;
        bool set;
    };
    Written *written_at(uint8_t index);
    void draw(uint8_t index, bool redraw);
    std::vector<Written> m_written_leds;
    uint8_t m_effect_type = 0;
    uint8_t m_effect_speed = 1;
    void track(uint32_t val)
    {
        m_written = (m_written ^ val) * 16777619u;
    }
    uint32_t m_written = NOTHING_WRITTEN;
    bool m_lit = false;
};
