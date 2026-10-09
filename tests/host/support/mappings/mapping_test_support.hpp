#pragma once
// Helpers for driving the real mapping classes: an input whose value a test sets, a profile,
// and a builder for the proto mapping config
#include <stdint.h>
#include <string.h>
#include <memory>
#include <type_traits>
#include <gtest/gtest.h>
#include "mappings/mapping.hpp"
// Profile owns these, so building one needs them complete
#include "leds/led_mappings.hpp"
#include "triggers/activation_trigger.hpp"
#include "pico/time.h"
#include "emulation/usb/hid_device.h"

class FakeInput : public Input
{
public:
    bool digital = false;
    uint16_t analog = 0;
    bool is_valid = true;
    bool independent_analog = false;

    bool tick_digital() override { return digital; }
    uint16_t tick_analog() override { return analog; }
    void setup() override {}
    bool valid() const override { return is_valid; }
    bool has_independent_analog_value() const override { return independent_analog; }
};

// A mapping under test, with the input it reads kept around so the test can change it
template <typename T>
struct Driven
{
    std::unique_ptr<T> mapping;
    FakeInput *input;

    T *operator->() { return mapping.get(); }

    // Sample the input as the main loop would, without talking to the config tool
    void set_analog(uint16_t value)
    {
        input->analog = value;
        mapping->update(false, false);
    }
    void set_digital(bool value)
    {
        input->digital = value;
        mapping->update(false, false);
    }
};

inline proto_Mapping base_config()
{
    proto_Mapping config;
    memset(&config, 0, sizeof(config));
    return config;
}

// Full range stick, centred in the middle, no deadzone
inline proto_Mapping stick_config()
{
    proto_Mapping config = base_config();
    config.has_min = config.has_max = config.has_center = true;
    config.min = 0;
    config.max = UINT16_MAX;
    config.center = UINT16_MAX / 2;
    return config;
}

// Full range trigger resting at 0, no deadzone
inline proto_Mapping trigger_config()
{
    proto_Mapping config = base_config();
    config.has_min = config.has_max = config.has_center = true;
    config.min = 0;
    config.max = UINT16_MAX;
    config.center = 0;
    return config;
}

inline proto_Mapping with_gamepad_axis(proto_Mapping config, GamepadAxisType axis)
{
    config.mapping.which_mapping = proto_Output_gamepadAxis_tag;
    config.mapping.mapping.gamepadAxis = axis;
    return config;
}

inline proto_Mapping gamepad_button(GamepadButtonType button)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_gamepadButton_tag;
    config.mapping.mapping.gamepadButton = button;
    return config;
}

inline std::shared_ptr<Profile> make_profile(SubType subtype = SubType_Gamepad)
{
    auto profile = std::make_shared<Profile>();
    profile->subtype = subtype;
    return profile;
}

template <typename T>
Driven<T> drive(const proto_Mapping &config, const std::shared_ptr<Profile> &profile)
{
    auto input = std::make_unique<FakeInput>();
    FakeInput *raw = input.get();
    return Driven<T>{std::make_unique<T>(config, std::move(input), 1, profile), raw};
}

// A zeroed report of the given type, passed to the mappings as bytes
template <typename R>
struct Report
{
    R data;
    Report() { memset(&data, 0, sizeof(data)); }
    uint8_t *buf() { return reinterpret_cast<uint8_t *>(&data); }
    const uint8_t *bytes() const { return reinterpret_cast<const uint8_t *>(&data); }
    R *operator->() { return &data; }
};

// Resets the fake clock and recorded config tool events between tests
class MappingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        fake_time::set_us(1000000);
        HIDConfigDevice::sent_events.clear();
    }
};
