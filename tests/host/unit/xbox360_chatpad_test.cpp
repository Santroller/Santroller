// The Xbox 360 chatpad's reports (protocols/xbox360_chatpad.hpp): scancodes as HID keycodes, and the green / orange
// legends, picked by holding or tapping the key "as the console does it"
#include <gtest/gtest.h>
#include "protocols/xbox360_chatpad.hpp"

namespace
{
constexpr uint8_t Q = 0x27;
constexpr uint8_t W = 0x26;
constexpr uint8_t A = 0x37;
constexpr uint8_t E = 0x25;

class ChatpadTest : public ::testing::Test
{
protected:
    Xbox360ChatpadDecoder chatpad;
    bool report(uint8_t modifiers, uint8_t key1 = 0, uint8_t key2 = 0)
    {
        const uint8_t buf[] = {0x00, modifiers, key1, key2, 0x00};
        return chatpad.handle_report(buf, sizeof(buf));
    }
    bool held(uint8_t keycode) { return chatpad.key_pressed(keycode); }
};
} // namespace

TEST_F(ChatpadTest, KeysAreHidKeycodes)
{
    EXPECT_TRUE(report(0, Q, W));
    EXPECT_TRUE(held(HID_KEY_Q));
    EXPECT_TRUE(held(HID_KEY_W));
    EXPECT_FALSE(report(0));
    EXPECT_FALSE(held(HID_KEY_Q));
}

TEST_F(ChatpadTest, OtherReportsAreIgnored)
{
    const uint8_t status[] = {0xF0, 0x03, 0x00, 0x00, 0x00};
    EXPECT_FALSE(chatpad.handle_report(status, sizeof(status)));
}

TEST_F(ChatpadTest, ColourKeysAreKeysAPcIgnores)
{
    report(chatpad_green | chatpad_orange | chatpad_people);
    EXPECT_TRUE(held(HID_KEY_F13));
    EXPECT_TRUE(held(HID_KEY_F14));
    EXPECT_TRUE(held(HID_KEY_F15));
    EXPECT_FALSE(held(HID_KEY_ALT_LEFT));
}

TEST_F(ChatpadTest, HoldingGreenTypesTheGreenLegend)
{
    report(chatpad_green);
    report(chatpad_green, Q);
    // !
    EXPECT_TRUE(held(HID_KEY_1));
    EXPECT_TRUE(held(HID_KEY_SHIFT_LEFT));
    EXPECT_FALSE(held(HID_KEY_Q));
}

TEST_F(ChatpadTest, TappingOrangePicksItForTheNextKeyOnly)
{
    report(chatpad_orange);
    report(0);
    EXPECT_EQ(chatpad.sticky(), chatpad_orange);
    report(0, 0x64);
    // =
    EXPECT_TRUE(held(HID_KEY_EQUAL));
    EXPECT_FALSE(held(HID_KEY_SHIFT_LEFT));
    EXPECT_EQ(chatpad.sticky(), 0);
    report(0);
    report(0, 0x64);
    EXPECT_TRUE(held(HID_KEY_P));
}

TEST_F(ChatpadTest, TappingTwiceCancels)
{
    report(chatpad_green);
    report(0);
    report(chatpad_green);
    report(0);
    EXPECT_EQ(chatpad.sticky(), 0);
    report(0, Q);
    EXPECT_TRUE(held(HID_KEY_Q));
}

TEST_F(ChatpadTest, AKeyKeepsWhatItTypedUntilLetGo)
{
    report(chatpad_green);
    report(chatpad_green, Q);
    report(0, Q);
    EXPECT_TRUE(held(HID_KEY_1));
    EXPECT_FALSE(held(HID_KEY_Q));
}

TEST_F(ChatpadTest, LegendsWithoutAUsKeyTypeNothing)
{
    report(chatpad_green);
    report(chatpad_green, E);
    EXPECT_TRUE(report(chatpad_green, E));
    EXPECT_FALSE(held(HID_KEY_E));
}

TEST_F(ChatpadTest, TappingShiftCapitalisesTheNextKey)
{
    report(chatpad_shift);
    report(0);
    report(0, A);
    EXPECT_TRUE(held(HID_KEY_A));
    EXPECT_TRUE(held(HID_KEY_SHIFT_LEFT));
    report(0);
    report(0, A);
    EXPECT_FALSE(held(HID_KEY_SHIFT_LEFT));
}

TEST_F(ChatpadTest, OrangeThenShiftIsCapsLock)
{
    report(chatpad_orange);
    report(0);
    report(chatpad_shift);
    EXPECT_TRUE(held(HID_KEY_CAPS_LOCK));
    EXPECT_FALSE(held(HID_KEY_SHIFT_LEFT));
    report(0);
    EXPECT_FALSE(held(HID_KEY_CAPS_LOCK));
    EXPECT_EQ(chatpad.sticky(), 0);
}
