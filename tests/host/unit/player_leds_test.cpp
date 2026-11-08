// player_led_pattern in include/leds/player_leds.hpp: which LEDs of one player LED mapping light for
// each player, matching the consoles' own controllers
#include <gtest/gtest.h>
#include "leds/player_leds.hpp"

TEST(PlayerLedPattern, NoPlayerLightsNothing)
{
    for (auto style : {PlayerLedSequential, PlayerLedPlayStation3, PlayerLedSwitch, PlayerLedPlayStation5})
    {
        EXPECT_EQ(player_led_pattern(style, 0), 0);
    }
}

TEST(PlayerLedPattern, SequentialLightsOneLedPerPlayer)
{
    EXPECT_EQ(player_led_pattern(PlayerLedSequential, 1), 0b0001);
    EXPECT_EQ(player_led_pattern(PlayerLedSequential, 4), 0b1000);
    EXPECT_EQ(player_led_pattern(PlayerLedSequential, 8), 0x80);
    EXPECT_EQ(player_led_pattern(PlayerLedSequential, 9), 0);
}

TEST(PlayerLedPattern, PlayStation3AddsTheFourthLedPastFourPlayers)
{
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation3, 4), 0b1000);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation3, 5), 0b1001);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation3, 7), 0b1100);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation3, 10), 0b1111);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation3, 11), 0);
}

TEST(PlayerLedPattern, SwitchCountsUpThenUsesCombinations)
{
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 1), 0b0001);
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 2), 0b0011);
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 3), 0b0111);
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 4), 0b1111);
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 5), 0b1001);
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 8), 0b0110);
    EXPECT_EQ(player_led_pattern(PlayerLedSwitch, 9), 0);
}

TEST(PlayerLedPattern, PlayStation5UsesFiveLedsFromTheCentreOut)
{
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation5, 1), 0b00100);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation5, 2), 0b01010);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation5, 3), 0b10101);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation5, 4), 0b11011);
    EXPECT_EQ(player_led_pattern(PlayerLedPlayStation5, 5), 0b11111);
}
