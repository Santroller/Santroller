#include <gtest/gtest.h>
#include <vector>
#include "protocols/stadia.hpp"
#include "protocols/proto_output.hpp"

// Google Stadia controller input report 3, shared by the USB and BLE hosts. Bit positions follow
// SDL's HIDAPI Stadia driver, as Stadia_Data_t does.

namespace
{
using Bytes = std::vector<uint8_t>;

Bytes report(uint8_t hat, uint8_t b2, uint8_t b3, size_t len = sizeof(Stadia_Data_t))
{
    Bytes r(len, 0);
    r[0] = STADIA_REPORT_ID;
    r[1] = hat;
    r[2] = b2;
    r[3] = b3;
    r[4] = 0x80;
    r[5] = 0x80;
    r[6] = 0x80;
    r[7] = 0x80;
    return r;
}

Stadia_Data_t parse(const Bytes &r)
{
    Stadia_Data_t data = {};
    EXPECT_TRUE(stadia_parse_report(r.data(), r.size(), data));
    return data;
}

bool pressed(const Stadia_Data_t &data, GamepadButtonType button)
{
    auto out = gamepad_button(button);
    return stadia_tick_digital(data, out);
}

uint16_t axis(const Stadia_Data_t &data, GamepadAxisType a)
{
    auto out = gamepad_axis(a);
    return stadia_tick_analog(data, out);
}
} // namespace

TEST(Stadia, EachButtonMapsToItsOwnOutput)
{
    struct Case
    {
        int byte;
        uint8_t mask;
        GamepadButtonType button;
    };
    const Case cases[] = {
        {2, 0x01, Gamepad_Capture},
        {2, 0x02, Gamepad_Misc},
        {2, 0x10, Gamepad_Guide},
        {2, 0x20, Gamepad_Start},
        {2, 0x40, Gamepad_Back},
        {2, 0x80, Gamepad_RightThumbClick},
        {3, 0x01, Gamepad_LeftThumbClick},
        {3, 0x02, Gamepad_RightShoulder},
        {3, 0x04, Gamepad_LeftShoulder},
        {3, 0x08, Gamepad_Y},
        {3, 0x10, Gamepad_X},
        {3, 0x20, Gamepad_B},
        {3, 0x40, Gamepad_A},
    };
    const GamepadButtonType all[] = {Gamepad_A, Gamepad_B, Gamepad_X, Gamepad_Y, Gamepad_Back, Gamepad_Start,
                                     Gamepad_Guide, Gamepad_Capture, Gamepad_LeftShoulder, Gamepad_RightShoulder,
                                     Gamepad_LeftThumbClick, Gamepad_RightThumbClick, Gamepad_Misc};
    for (const auto &c : cases)
    {
        auto data = parse(report(8, c.byte == 2 ? c.mask : 0, c.byte == 3 ? c.mask : 0));
        for (auto b : all)
        {
            EXPECT_EQ(pressed(data, b), b == c.button) << "byte " << c.byte << " mask " << int(c.mask) << " button " << b;
        }
    }
}

TEST(Stadia, DigitalTriggersAreReadThroughTheTriggerAxes)
{
    auto none = parse(report(8, 0, 0));
    auto lt = gamepad_axis(Gamepad_LeftTrigger);
    auto rt = gamepad_axis(Gamepad_RightTrigger);
    auto lx = gamepad_axis(Gamepad_LeftStickX);
    EXPECT_FALSE(stadia_tick_digital(none, lt));
    EXPECT_FALSE(stadia_tick_digital(none, rt));
    auto left = parse(report(8, 0x08, 0));
    EXPECT_TRUE(stadia_tick_digital(left, lt));
    EXPECT_FALSE(stadia_tick_digital(left, rt));
    auto right = parse(report(8, 0x04, 0));
    EXPECT_FALSE(stadia_tick_digital(right, lt));
    EXPECT_TRUE(stadia_tick_digital(right, rt));
    auto both = parse(report(8, 0x0C, 0));
    EXPECT_FALSE(stadia_tick_digital(both, lx));
}

TEST(Stadia, DigitalTriggersPressNoButtons)
{
    auto data = parse(report(8, 0x04 | 0x08, 0));
    for (int b = Gamepad_A; b <= Gamepad_Misc; b++)
    {
        EXPECT_FALSE(pressed(data, GamepadButtonType(b))) << b;
    }
}

TEST(Stadia, Hat)
{
    struct Case
    {
        uint8_t hat;
        bool up, right, down, left;
    };
    const Case cases[] = {
        {0, true, false, false, false}, {1, true, true, false, false}, {2, false, true, false, false},
        {3, false, true, true, false},  {4, false, false, true, false}, {5, false, false, true, true},
        {6, false, false, false, true}, {7, true, false, false, true},  {8, false, false, false, false},
        {15, false, false, false, false},
    };
    for (const auto &c : cases)
    {
        auto data = parse(report(c.hat, 0, 0));
        EXPECT_EQ(pressed(data, Gamepad_DpadUp), c.up) << int(c.hat);
        EXPECT_EQ(pressed(data, Gamepad_DpadRight), c.right) << int(c.hat);
        EXPECT_EQ(pressed(data, Gamepad_DpadDown), c.down) << int(c.hat);
        EXPECT_EQ(pressed(data, Gamepad_DpadLeft), c.left) << int(c.hat);
    }
}

TEST(Stadia, Axes)
{
    auto r = report(8, 0, 0);
    r[4] = 0xFF; // left x, right
    r[5] = 0x00; // left y, up
    r[6] = 0x00; // right x, left
    r[7] = 0xFF; // right y, down
    r[8] = 0xFF;
    r[9] = 0x00;
    auto data = parse(r);
    EXPECT_EQ(axis(data, Gamepad_LeftStickX), 0xFFFF);
    EXPECT_EQ(axis(data, Gamepad_LeftStickY), 0xFFFF);
    EXPECT_EQ(axis(data, Gamepad_RightStickX), 0x0000);
    EXPECT_EQ(axis(data, Gamepad_RightStickY), 0x0000);
    EXPECT_EQ(axis(data, Gamepad_LeftTrigger), 0xFFFF);
    EXPECT_EQ(axis(data, Gamepad_RightTrigger), 0x0000);
}

// The USB host hands over a whole 64 byte packet buffer, which must not be copied past the report
TEST(Stadia, OversizedPacketOnlyCopiesTheReport)
{
    struct
    {
        Stadia_Data_t data;
        uint8_t guard[64];
    } target;
    memset(&target, 0, sizeof(target));
    auto r = report(8, 0, 0x40, 64);
    memset(r.data() + sizeof(Stadia_Data_t), 0xAA, 64 - sizeof(Stadia_Data_t));
    EXPECT_TRUE(stadia_parse_report(r.data(), r.size(), target.data));
    EXPECT_TRUE(pressed(target.data, Gamepad_A));
    for (uint8_t b : target.guard)
    {
        EXPECT_EQ(b, 0);
    }
}

TEST(Stadia, IgnoresOtherReportsAndShortReports)
{
    Stadia_Data_t data = {};
    data.a = 1;
    auto other = report(8, 0, 0);
    other[0] = 5;
    EXPECT_FALSE(stadia_parse_report(other.data(), other.size(), data));
    auto shortr = report(8, 0, 0);
    shortr.resize(sizeof(Stadia_Data_t) - 1);
    EXPECT_FALSE(stadia_parse_report(shortr.data(), shortr.size(), data));
    EXPECT_TRUE(data.a);
}

TEST(Stadia, IgnoresNonGamepadMappings)
{
    auto data = parse(report(0, 0xFF, 0xFF));
    auto out = non_gamepad_mapping(Gamepad_A);
    EXPECT_FALSE(stadia_tick_digital(data, out));
    EXPECT_EQ(stadia_tick_analog(data, out), 0);
}

TEST(Stadia, RumbleReportIsStrongThenWeakLittleEndian)
{
    uint8_t out[STADIA_RUMBLE_LEN];
    stadia_build_rumble(0xFF, 0x12, out);
    EXPECT_EQ(out[0], 0xFF);
    EXPECT_EQ(out[1], 0xFF);
    EXPECT_EQ(out[2], 0x12);
    EXPECT_EQ(out[3], 0x12);
    stadia_build_rumble(0, 0, out);
    for (uint8_t b : out)
    {
        EXPECT_EQ(b, 0);
    }
}

TEST(Stadia, RumbleSendsOnConnectThenOnlyOnChanges)
{
    StadiaRumble r;
    EXPECT_TRUE(r.should_send(100));
    r.sent(100);
    EXPECT_FALSE(r.should_send(5000));
    r.set(0, 0);
    EXPECT_FALSE(r.should_send(5000));
    r.set(10, 20);
    // rate limited right after a send
    EXPECT_FALSE(r.should_send(100 + STADIA_OUTPUT_MIN_INTERVAL_MS - 1));
    EXPECT_TRUE(r.should_send(100 + STADIA_OUTPUT_MIN_INTERVAL_MS));
    r.sent(200);
    // a held level isn't resent, the motors keep running on their own
    EXPECT_FALSE(r.should_send(10000));
}
