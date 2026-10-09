#include <gtest/gtest.h>
#include "protocols/rb_pickup.hpp"

TEST(RbPickup, NotchValuesUseTheUniversalTableAsTheHighByte)
{
    for (uint8_t notch = 0; notch <= 4; notch++)
    {
        uint16_t value = rb_pickup_notch_value(notch);
        EXPECT_EQ(value >> 8, rb_pickup_universal[notch]) << int(notch);
        EXPECT_EQ(value & 0xFF, 0) << int(notch);
    }
}

TEST(RbPickup, NotchValuesIncreaseWithTheNotch)
{
    for (uint8_t notch = 1; notch <= 4; notch++)
        EXPECT_GT(rb_pickup_notch_value(notch), rb_pickup_notch_value(notch - 1)) << int(notch);
}

TEST(RbPickup, OutOfRangeNotchesClampToTheLast)
{
    EXPECT_EQ(rb_pickup_notch_value(5), rb_pickup_notch_value(4));
    EXPECT_EQ(rb_pickup_notch_value(0xFF), rb_pickup_notch_value(4));
}
