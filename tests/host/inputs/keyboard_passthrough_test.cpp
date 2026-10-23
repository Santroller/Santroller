// Keyboard passthrough (Output.keyboardPassthrough): "Every key held on the USB keyboard the input reads from,
// apart from keys other mappings read from it", which Profile::resolve_keyboard_passthrough works out from the
// inputs' hardware ids.
#include "inputs_test_support.hpp"
#include "class/hid/hid.h"
#include "mappings/peripheral_mappings.hpp"

namespace
{
// A USB button input's hardware id, as USBButtonInput::hardware_id builds it
uint64_t usb_button_id(int32_t deviceid, pb_size_t tag, uint8_t value)
{
    return (static_cast<uint64_t>(InputHw_USBButton) << 56) | (static_cast<uint64_t>(deviceid) << 32) |
           (static_cast<uint64_t>(tag) << 16) | value;
}

// A keyboard's keys, read through a passthrough input
class TestKeyboardInput : public TestInput
{
public:
    using TestInput::TestInput;
    bool keys[256] = {};
    bool key_pressed(uint8_t keycode) override { return keys[keycode]; }
};

proto_Mapping output(pb_size_t tag, int32_t value)
{
    proto_Mapping config;
    memset(&config, 0, sizeof(config));
    config.mapping.which_mapping = tag;
    config.mapping.mapping.keycode = value;
    return config;
}

class KeyboardPassthroughTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(KeyboardMouse);
    TestKeyboardInput *keyboard = nullptr;

    // A passthrough of the keyboard in slot deviceid
    void add_passthrough(int32_t deviceid)
    {
        auto input = std::make_unique<TestKeyboardInput>(usb_button_id(deviceid, proto_Output_keyboardPassthrough_tag, 1));
        keyboard = input.get();
        profile->mappings.push_back(std::make_unique<KeyboardPassthroughMapping>(
            output(proto_Output_keyboardPassthrough_tag, 1), std::move(input), (uint16_t)profile->mappings.size(), profile));
    }
    // A key read from the keyboard in slot deviceid, sending another key
    void add_remap(int32_t deviceid, uint8_t from, uint8_t to)
    {
        TestInput *input = nullptr;
        profile->mappings.push_back(std::make_unique<KeyboardButtonMapping>(
            output(proto_Output_keycode_tag, to), test_input(input, usb_button_id(deviceid, proto_Output_keycode_tag, from)),
            (uint16_t)profile->mappings.size(), profile));
    }
    // The keys a report built now would send
    void report()
    {
        profile->keyboard_state.clear_all();
        for (auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
            mapping->update_hid(nullptr);
        }
    }
    bool sends(uint8_t keycode) { return profile->keyboard_state.is_key_pressed(keycode); }
};
} // namespace

TEST_F(KeyboardPassthroughTest, SendsEveryHeldKey)
{
    add_passthrough(1);
    profile->resolve_keyboard_passthrough();
    keyboard->keys[HID_KEY_A] = true;
    keyboard->keys[HID_KEY_SHIFT_LEFT] = true;
    report();
    EXPECT_TRUE(sends(HID_KEY_A));
    EXPECT_TRUE(sends(HID_KEY_SHIFT_LEFT));
    EXPECT_FALSE(sends(HID_KEY_B));
}

TEST_F(KeyboardPassthroughTest, RemappedKeysAreLeftToTheirMapping)
{
    add_passthrough(1);
    add_remap(1, HID_KEY_A, HID_KEY_B);
    profile->resolve_keyboard_passthrough();
    keyboard->keys[HID_KEY_A] = true;
    keyboard->keys[HID_KEY_C] = true;
    report();
    EXPECT_FALSE(sends(HID_KEY_A));
    EXPECT_TRUE(sends(HID_KEY_C));
}

TEST_F(KeyboardPassthroughTest, KeysReadFromAnotherKeyboardStillPassThrough)
{
    add_passthrough(1);
    add_remap(2, HID_KEY_A, HID_KEY_B);
    profile->resolve_keyboard_passthrough();
    keyboard->keys[HID_KEY_A] = true;
    report();
    EXPECT_TRUE(sends(HID_KEY_A));
}
