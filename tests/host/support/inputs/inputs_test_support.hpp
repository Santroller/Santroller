#pragma once
// Helpers for the input modifier, trigger and profile tests: an input whose state a test sets,
// a device the device triggers can match, LED doubles and a fixture that resets every fake
#include <stdint.h>
#include <string.h>
#include <memory>
#include <vector>
#include <gtest/gtest.h>
#include "mappings/mapping.hpp"
#include "leds/led_mappings.hpp"
#include "triggers/activation_trigger.hpp"
#include "triggers/activation_trigger_list.hpp"
#include "managers/config_manager.hpp"
#include "managers/device_manager.hpp"
#include "managers/profile_manager.hpp"
#include "config/config.hpp"
#include "pico/time.h"
#include "pico/bootrom.h"
#include "emulation/usb/hid_device.h"
#include "fake_devices.hpp"

// An input a test drives directly. hw is what shortcut masking matches inputs by.
class TestInput : public Input
{
public:
    explicit TestInput(uint64_t hw = 0) : hw(hw) {}
    bool digital = false;
    uint16_t analog = 0;
    uint64_t hw = 0;
    bool is_valid = true;
    bool independent_analog = false;
    int setups = 0;
    int digital_reads = 0;
    int linked = 0;
    int linked_claimed = 0;

    bool tick_digital() override
    {
        digital_reads++;
        return digital;
    }
    uint16_t tick_analog() override { return analog; }
    void setup() override { setups++; }
    bool valid() const override { return is_valid; }
    bool has_independent_analog_value() const override { return independent_analog; }
    uint64_t hardware_id() const override { return hw; }
    void link_device(bool claim) override
    {
        linked++;
        if (claim)
        {
            linked_claimed++;
        }
    }
    void set(bool pressed)
    {
        digital = pressed;
        analog = pressed ? UINT16_MAX : 0;
    }
};

// Makes a TestInput, keeping a pointer to it for the test after ownership moves away
inline std::unique_ptr<TestInput> test_input(TestInput *&out, uint64_t hw = 0)
{
    auto input = std::make_unique<TestInput>(hw);
    out = input.get();
    return input;
}

// A GPIO pin's hardware id, as GPIOInput::hardware_id builds it
inline uint64_t pin_id(uint8_t pin)
{
    return (static_cast<uint64_t>(InputHw_GPIO) << 56) | pin;
}

// Physical buttons, so several inputs (a shortcut's members and a plain mapping) can read one pin
namespace fake_pins
{
inline bool state[32] = {};
inline void reset()
{
    for (auto &pin : state)
    {
        pin = false;
    }
}
}

// A digital button on a GPIO pin, with the same hardware id GPIOInput gives that pin
class PinInput : public Input
{
public:
    explicit PinInput(uint8_t pin) : m_pin(pin) {}
    bool tick_digital() override { return fake_pins::state[m_pin]; }
    uint16_t tick_analog() override { return fake_pins::state[m_pin] ? UINT16_MAX : 0; }
    void setup() override {}
    uint64_t hardware_id() const override { return pin_id(m_pin); }

private:
    uint8_t m_pin;
};

inline std::unique_ptr<Input> pin(uint8_t number)
{
    return std::make_unique<PinInput>(number);
}

namespace clock_ms
{
inline uint32_t now() { return (uint32_t)(fake_time::now_us / 1000); }
inline void set(uint64_t ms) { fake_time::set_us(ms * 1000); }
inline void advance(uint64_t ms) { fake_time::advance_us(ms * 1000); }
}

// A device the device type triggers can find, matching whatever a test says it is
class TestDevice : public Device
{
public:
    explicit TestDevice(uint16_t id) : Device(id) { still_connected = true; }
    WiiExtType wii_type = WiiNoExtension;
    PS2ControllerType ps2_type = PS2ControllerTypeUnknown;
    SubType usb_type = SubType(0);
    SubType bt_type = SubType(0);
    // vid / pid, 0 for none
    uint32_t vid = 0;
    uint32_t pid = 0;
    // Bit n set: MIDI channel n (0 based, as on the wire) has been seen
    uint32_t midi_channels = 0;
    int player_led = -1;

    void begin() override {}
    void end(bool) override {}
    void update(bool, bool) override {}
    bool using_pin(uint8_t) override { return false; }
    bool is_wii_extension(WiiExtType type) override { return wii_type != WiiNoExtension && type == wii_type; }
    bool is_ps2_device(PS2ControllerType type) override { return ps2_type != PS2ControllerTypeUnknown && type == ps2_type; }
    bool is_usb_type(SubType type) override { return usb_type && type == usb_type; }
    bool is_bluetooth_type(SubType type) override { return bt_type && type == bt_type; }
    bool is_usb_device(proto_SpecificUsbDevice type) override { return vid && type.vid == vid && type.pid == pid; }
    bool is_bluetooth_device(proto_SpecificBluetoothDevice type) override { return vid && type.vid == vid && type.pid == pid; }
    bool has_midi_channel(uint8_t channel) override { return channel < 32 && (midi_channels & (1u << channel)); }
    void set_player_led(uint8_t player) override { player_led = player; }
};

// LED output whose off() calls are counted
class CountingLedDevice : public LedMappingDevice
{
public:
    explicit CountingLedDevice(int *offs) : m_offs(offs) {}
    void setup() override {}
    void off() override { (*m_offs)++; }
    bool supports_brightness() override { return false; }
    uint8_t led_count() override { return 1; }

protected:
    void write_val(uint16_t) override {}
    void write_val_raw(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) override {}

private:
    int *m_offs;
};

class TestLedMapping : public LedMapping
{
public:
    using LedMapping::LedMapping;
    void update(bool, bool) override {}
    void reload() override {}
};

inline proto_Mapping button_config(GamepadButtonType button)
{
    proto_Mapping config;
    memset(&config, 0, sizeof(config));
    config.mapping.which_mapping = proto_Output_gamepadButton_tag;
    config.mapping.mapping.gamepadButton = button;
    return config;
}

inline proto_Mapping with_debounce_ms(proto_Mapping config, uint32_t ms)
{
    config.has_debounce = true;
    config.debounce = ms;
    return config;
}

inline std::shared_ptr<Profile> make_profile(SubType subtype = SubType_Gamepad)
{
    auto profile = std::make_shared<Profile>();
    profile->subtype = subtype;
    profile->mode = ModeHid;
    profile->profile_id = 1;
    return profile;
}

// Resets the clock and every fake between tests. The clock starts well after boot, and the
// console mode was last changed at boot, so "recently changed" is false until a test says so.
class InputsTest : public ::testing::Test
{
protected:
    static constexpr uint64_t START_MS = 100000;
    void SetUp() override
    {
        clock_ms::set(START_MS);
        HIDConfigDevice::sent_events.clear();
        fake_config::reset();
        fake_devices::reset();
        fake_bootrom::reset_count = 0;
        fake_pins::reset();
        ConfigManager::instance().clear_all();
        DeviceManager::instance().clear_assignable_devices();
        ProfileManager::instance().instances.clear();
    }
    void TearDown() override
    {
        DeviceManager::instance().clear_assignable_devices();
        ProfileManager::instance().instances.clear();
    }
};
