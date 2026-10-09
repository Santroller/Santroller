#include <gtest/gtest.h>
#include <cstddef>
#include <cstring>
#include "protocols/switch_arcade.hpp"

// HORI style Switch HID report: 16 button bits, hat (0 = up, clockwise, 8 = neutral), four 8 bit axes, vendor byte

static_assert(offsetof(SwitchArcadeReport, hat) == 2);
static_assert(offsetof(SwitchArcadeReport, lx) == 3);
static_assert(offsetof(SwitchArcadeReport, ry) == 6);

TEST(SwitchArcade, HoriIds)
{
    EXPECT_EQ(SWITCH_ARCADE_VID, 0x0F0D); // HORI
}

TEST(SwitchArcade, ButtonOrderMatchesTheHoriPad)
{
    EXPECT_EQ(SwitchArcade_Y, 0x0001);
    EXPECT_EQ(SwitchArcade_B, 0x0002);
    EXPECT_EQ(SwitchArcade_A, 0x0004);
    EXPECT_EQ(SwitchArcade_X, 0x0008);
    EXPECT_EQ(SwitchArcade_L, 0x0010);
    EXPECT_EQ(SwitchArcade_R, 0x0020);
    EXPECT_EQ(SwitchArcade_ZL, 0x0040);
    EXPECT_EQ(SwitchArcade_ZR, 0x0080);
    EXPECT_EQ(SwitchArcade_Minus, 0x0100);
    EXPECT_EQ(SwitchArcade_Plus, 0x0200);
    EXPECT_EQ(SwitchArcade_LS, 0x0400);
    EXPECT_EQ(SwitchArcade_RS, 0x0800);
    EXPECT_EQ(SwitchArcade_Home, 0x1000);
    EXPECT_EQ(SwitchArcade_Capture, 0x2000);
}

TEST(SwitchArcade, ButtonsAreLittleEndian)
{
    SwitchArcadeReport r{};
    r.buttons_low = 0x34;
    r.buttons_high = 0x12;
    EXPECT_EQ(switch_arcade_buttons(r), 0x1234);
}

TEST(SwitchArcade, SetButtonOrsIntoTheRightByte)
{
    SwitchArcadeReport r{};
    switch_arcade_set_button(r, SwitchArcade_B);
    EXPECT_EQ(r.buttons_low, 0x02);
    EXPECT_EQ(r.buttons_high, 0x00);
    switch_arcade_set_button(r, SwitchArcade_Home);
    EXPECT_EQ(r.buttons_low, 0x02);
    EXPECT_EQ(r.buttons_high, 0x10);
    // setting a button twice, or a mask of several, only ever adds bits
    switch_arcade_set_button(r, SwitchArcade_B | SwitchArcade_Minus);
    EXPECT_EQ(switch_arcade_buttons(r), SwitchArcade_B | SwitchArcade_Home | SwitchArcade_Minus);
    // nothing else in the report is touched
    EXPECT_EQ(r.hat, 0);
    EXPECT_EQ(r.lx, 0);
}

TEST(SwitchArcade, FinishHatMapsEveryDirectionMask)
{
    constexpr uint8_t U = 1, D = 2, L = 4, R = 8;
    struct
    {
        uint8_t mask;
        uint8_t hat;
    } cases[] = {
        {0, 8},
        {U, 0},
        {U | R, 1},
        {R, 2},
        {D | R, 3},
        {D, 4},
        {D | L, 5},
        {L, 6},
        {U | L, 7},
        // opposite directions cancel out
        {U | D, 8},
        {L | R, 8},
        {U | D | L | R, 8},
        {U | D | R, 2},
        {U | D | L, 6},
        {U | L | R, 0},
        {D | L | R, 4},
    };
    for (auto c : cases)
    {
        SwitchArcadeReport r{};
        r.hat = c.mask << 4;
        switch_arcade_finish_hat(r);
        EXPECT_EQ(r.hat, c.hat) << "mask " << int(c.mask);
    }
}

TEST(SwitchArcade, FinishHatIgnoresTheLowNibble)
{
    SwitchArcadeReport r{};
    r.hat = 0x0F;
    switch_arcade_finish_hat(r);
    EXPECT_EQ(r.hat, 8);
    r.hat = (1 << 4) | 0x0A;
    switch_arcade_finish_hat(r);
    EXPECT_EQ(r.hat, 0);
}
