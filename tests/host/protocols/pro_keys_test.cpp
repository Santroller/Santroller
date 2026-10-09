#include <gtest/gtest.h>
#include "protocols/pro_keys.hpp"

// Rock Band 3 Pro Keyboard: three bytes of key bits, highest bit first (key 1 = bit 7 of the first byte),
// then five velocity bytes, one for each of the first five held keys from the lowest. Key 25 (the top C)
// is bit 7 of the first velocity byte.

namespace
{
struct Keys
{
    uint8_t key1 = 0, key2 = 0, key3 = 0;
    uint8_t velocities[5] = {};

    Keys &hold(int key)
    {
        if (key <= 8)
            key1 |= 1u << (8 - key);
        else if (key <= 16)
            key2 |= 1u << (16 - key);
        else if (key <= 24)
            key3 |= 1u << (24 - key);
        else
            velocities[0] |= 0x80;
        return *this;
    }
    bool pressed(int32_t key) const { return pro_keyboard_key_pressed(key1, key2, key3, velocities, key); }
    bool any(int32_t count) const { return pro_keyboard_any_key_pressed(key1, key2, key3, velocities, count); }
    uint16_t pressure(int32_t key) const { return pro_keyboard_key_pressure(key1, key2, key3, velocities, key); }
    uint16_t range(int32_t count) const { return pro_keyboard_key_range_pressure(key1, key2, key3, velocities, count); }
};
} // namespace

TEST(ProKeys, KeyBitsAreHighestBitFirst)
{
    for (int key = 1; key <= 25; key++)
    {
        Keys k;
        k.hold(key);
        for (int other = 1; other <= 25; other++)
            EXPECT_EQ(k.pressed(other), other == key) << "holding " << key << " checking " << other;
    }
    Keys k;
    k.key1 = 0x80;
    EXPECT_TRUE(k.pressed(1));
    k = {};
    k.key1 = 0x01;
    EXPECT_TRUE(k.pressed(8));
    k = {};
    k.key2 = 0x80;
    EXPECT_TRUE(k.pressed(9));
    k = {};
    k.key3 = 0x01;
    EXPECT_TRUE(k.pressed(24));
    k = {};
    k.velocities[0] = 0x80;
    EXPECT_TRUE(k.pressed(25));
}

TEST(ProKeys, OutOfRangeKeysAreNeverPressed)
{
    Keys k;
    k.key1 = k.key2 = k.key3 = 0xFF;
    for (auto &v : k.velocities)
        v = 0xFF;
    EXPECT_FALSE(k.pressed(0));
    EXPECT_FALSE(k.pressed(-1));
    EXPECT_FALSE(k.pressed(26));
    EXPECT_FALSE(k.pressed(INT32_MAX));
    EXPECT_FALSE(k.pressed(INT32_MIN));
}

TEST(ProKeys, VelocityBitsOtherThanKey25AreNotKeys)
{
    Keys k;
    k.velocities[0] = 0x7F;
    k.velocities[1] = 0xFF;
    for (int key = 1; key <= 25; key++)
        EXPECT_FALSE(k.pressed(key)) << key;
}

TEST(ProKeys, AnyKeyPressedHonoursTheCount)
{
    Keys k;
    EXPECT_FALSE(k.any(25));
    k.hold(9);
    EXPECT_FALSE(k.any(8));
    EXPECT_TRUE(k.any(9));
    EXPECT_TRUE(k.any(25));
    EXPECT_FALSE(k.any(0));
    EXPECT_FALSE(k.any(-5));
    // the count is clamped to the 25 keys
    Keys top;
    top.hold(25);
    EXPECT_TRUE(top.any(25));
    EXPECT_TRUE(top.any(1000));
    EXPECT_FALSE(top.any(24));
}

TEST(ProKeys, PressureOfAReleasedKeyIsZero)
{
    Keys k;
    k.velocities[0] = 0x7F;
    EXPECT_EQ(k.pressure(1), 0);
    EXPECT_EQ(k.pressure(0), 0);
    EXPECT_EQ(k.pressure(26), 0);
}

TEST(ProKeys, SingleKeyUsesTheFirstVelocity)
{
    Keys k;
    k.hold(5);
    k.velocities[0] = 0x40;
    EXPECT_EQ(k.pressure(5), 0x40 << 9);
    k.velocities[0] = 0x7F;
    EXPECT_EQ(k.pressure(5), 0xFE00);
    k.velocities[0] = 0x01;
    EXPECT_EQ(k.pressure(5), 0x0200);
}

TEST(ProKeys, VelocitiesFollowHeldKeysFromTheLowest)
{
    Keys k;
    k.hold(3).hold(10).hold(20);
    k.velocities[0] = 10;
    k.velocities[1] = 20;
    k.velocities[2] = 30;
    EXPECT_EQ(k.pressure(3), 10 << 9);
    EXPECT_EQ(k.pressure(10), 20 << 9);
    EXPECT_EQ(k.pressure(20), 30 << 9);
}

TEST(ProKeys, Key25VelocityIgnoresItsOwnFlag)
{
    Keys k;
    k.hold(25);
    k.velocities[0] |= 0x55;
    EXPECT_EQ(k.pressure(25), 0x55 << 9);
    // with a lower key held too, key 25 takes the second slot
    Keys both;
    both.hold(1).hold(25);
    both.velocities[0] |= 0x11;
    both.velocities[1] = 0x22;
    EXPECT_EQ(both.pressure(1), 0x11 << 9);
    EXPECT_EQ(both.pressure(25), 0x22 << 9);
}

TEST(ProKeys, KeysPastTheFifthGetTheDefaultVelocity)
{
    Keys k;
    for (int key = 1; key <= 7; key++)
        k.hold(key);
    for (int i = 0; i < 5; i++)
        k.velocities[i] = 0x10 + i;
    for (int key = 1; key <= 5; key++)
        EXPECT_EQ(k.pressure(key), (0x10 + key - 1) << 9) << key;
    EXPECT_EQ(k.pressure(6), PRO_KEYBOARD_DEFAULT_VELOCITY << 9);
    EXPECT_EQ(k.pressure(7), PRO_KEYBOARD_DEFAULT_VELOCITY << 9);
}

TEST(ProKeys, RangePressureIsTheHardestKey)
{
    Keys k;
    EXPECT_EQ(k.range(25), 0);
    k.hold(2).hold(12);
    k.velocities[0] = 0x20;
    k.velocities[1] = 0x60;
    EXPECT_EQ(k.range(25), 0x60 << 9);
    // only keys inside the range count
    EXPECT_EQ(k.range(11), 0x20 << 9);
    EXPECT_EQ(k.range(1), 0);
    EXPECT_EQ(k.range(0), 0);
    EXPECT_EQ(k.range(1000), 0x60 << 9);
}
