#include <gtest/gtest.h>
#include "mappings/calibration.hpp"

// calibrate_axis takes (value, max, min, deadzone, center, trigger), in that order.
// Outputs are the full 0 - 65535 range: a stick rests at 32767 (UINT16_MAX / 2), a trigger at 0.
namespace
{
constexpr uint16_t kStickCentre = UINT16_MAX / 2;

uint16_t trigger(float val, float min, float max, float deadzone = 0)
{
    return calibrate_axis(val, max, min, deadzone, 0, true);
}

uint16_t stick(float val, float min, float max, float center, float deadzone = 0)
{
    return calibrate_axis(val, max, min, deadzone, center, false);
}
} // namespace

TEST(CalibrateTrigger, FullRangeEndsMapToOutputEnds)
{
    EXPECT_EQ(trigger(0, 0, UINT16_MAX), 0);
    EXPECT_EQ(trigger(UINT16_MAX, 0, UINT16_MAX), UINT16_MAX);
}

TEST(CalibrateTrigger, FullRangeIsIdentity)
{
    EXPECT_EQ(trigger(1, 0, UINT16_MAX), 1);
    EXPECT_EQ(trigger(32768, 0, UINT16_MAX), 32768);
    EXPECT_EQ(trigger(UINT16_MAX - 1, 0, UINT16_MAX), UINT16_MAX - 1);
}

TEST(CalibrateTrigger, NarrowRangeIsStretchedToFullOutput)
{
    // 1000 - 61000 is 60000 wide, so half way is 31000
    EXPECT_EQ(trigger(1000, 1000, 61000), 0);
    EXPECT_NEAR(trigger(31000, 1000, 61000), 32767, 1);
    EXPECT_EQ(trigger(61000, 1000, 61000), UINT16_MAX);
}

TEST(CalibrateTrigger, BelowMinimumIsZero)
{
    EXPECT_EQ(trigger(0, 1000, 61000), 0);
    EXPECT_EQ(trigger(999, 1000, 61000), 0);
}

TEST(CalibrateTrigger, PastMaximumClampsToFull)
{
    EXPECT_EQ(trigger(61001, 1000, 61000), UINT16_MAX);
    EXPECT_EQ(trigger(UINT16_MAX, 1000, 61000), UINT16_MAX);
}

TEST(CalibrateTrigger, DeadzoneMovesTheStartPoint)
{
    // min + deadzone = 1500 is the new zero point, and the range still ends at max
    EXPECT_EQ(trigger(1499, 1000, 61500, 500), 0);
    EXPECT_EQ(trigger(1500, 1000, 61500, 500), 0);
    EXPECT_NEAR(trigger(31500, 1000, 61500, 500), 32767, 1);
    EXPECT_EQ(trigger(61500, 1000, 61500, 500), UINT16_MAX);
}

TEST(CalibrateTrigger, JustPastDeadzoneIsSmallButNonZero)
{
    // 1 step into a 60000 wide range is about 1.09 output steps
    EXPECT_EQ(trigger(1501, 1000, 61500, 500), 1);
}

TEST(CalibrateTrigger, InvertedRunsFromMinDownToMax)
{
    // min > max: the trigger rests at the high raw value
    EXPECT_EQ(trigger(61000, 61000, 1000), 0);
    EXPECT_EQ(trigger(1000, 61000, 1000), UINT16_MAX);
    EXPECT_NEAR(trigger(31000, 61000, 1000), 32767, 1);
}

TEST(CalibrateTrigger, InvertedOutsideRangeClamps)
{
    EXPECT_EQ(trigger(65000, 61000, 1000), 0);
    EXPECT_EQ(trigger(0, 61000, 1000), UINT16_MAX);
}

TEST(CalibrateTrigger, InvertedDeadzoneCountsDownFromMin)
{
    // the deadzone sits below min when inverted, so 60500 is the new zero point
    EXPECT_EQ(trigger(60600, 61000, 1000, 500), 0);
    EXPECT_EQ(trigger(60500, 61000, 1000, 500), 0);
    EXPECT_EQ(trigger(1000, 61000, 1000, 500), UINT16_MAX);
}

TEST(CalibrateStick, CentreAndEnds)
{
    EXPECT_EQ(stick(32767, 0, UINT16_MAX, 32767), kStickCentre);
    EXPECT_EQ(stick(0, 0, UINT16_MAX, 32767), 0);
    EXPECT_EQ(stick(UINT16_MAX, 0, UINT16_MAX, 32767), UINT16_MAX);
}

TEST(CalibrateStick, EachHalfIsScaledSeparately)
{
    // an off centre stick: 0 - 20000 is the low half and 20000 - 65535 the high half
    EXPECT_EQ(stick(20000, 0, UINT16_MAX, 20000), kStickCentre);
    EXPECT_NEAR(stick(10000, 0, UINT16_MAX, 20000), 16383, 1);
    // half way up the high half: 20000 + 45535 / 2
    EXPECT_NEAR(stick(42767, 0, UINT16_MAX, 20000), 49151, 1);
}

TEST(CalibrateStick, OutsideRangeClamps)
{
    EXPECT_EQ(stick(0, 2000, 62000, 32000), 0);
    EXPECT_EQ(stick(1999, 2000, 62000, 32000), 0);
    EXPECT_EQ(stick(62001, 2000, 62000, 32000), UINT16_MAX);
    EXPECT_EQ(stick(UINT16_MAX, 2000, 62000, 32000), UINT16_MAX);
}

TEST(CalibrateStick, InsideDeadzoneIsCentred)
{
    EXPECT_EQ(stick(32768 + 999, 0, UINT16_MAX, 32768, 1000), kStickCentre);
    EXPECT_EQ(stick(32768 - 999, 0, UINT16_MAX, 32768, 1000), kStickCentre);
}

TEST(CalibrateStick, DeadzoneEdgeIsStillCentreSoThereIsNoJump)
{
    // the scaled range starts at the deadzone edge, so leaving the deadzone is continuous
    EXPECT_EQ(stick(32768 + 1000, 0, UINT16_MAX, 32768, 1000), kStickCentre);
    EXPECT_EQ(stick(32768 - 1000, 0, UINT16_MAX, 32768, 1000), kStickCentre);
    EXPECT_GT(stick(32768 + 1100, 0, UINT16_MAX, 32768, 1000), kStickCentre);
    EXPECT_LT(stick(32768 - 1100, 0, UINT16_MAX, 32768, 1000), kStickCentre);
}

TEST(CalibrateStick, DeadzoneStillReachesTheEnds)
{
    EXPECT_EQ(stick(0, 0, UINT16_MAX, 32768, 1000), 0);
    EXPECT_EQ(stick(UINT16_MAX, 0, UINT16_MAX, 32768, 1000), UINT16_MAX);
}

TEST(CalibrateStick, InvertedSwapsTheEnds)
{
    // min > max: the raw maximum is the output minimum
    EXPECT_EQ(stick(UINT16_MAX, UINT16_MAX, 0, 32768), 0);
    EXPECT_EQ(stick(0, UINT16_MAX, 0, 32768), UINT16_MAX);
    EXPECT_EQ(stick(32768, UINT16_MAX, 0, 32768), kStickCentre);
}

TEST(CalibrateStick, InvertedLowHalfGoesUp)
{
    // a quarter of the way down from centre on an inverted stick is a quarter of the way up
    EXPECT_NEAR(stick(16384, UINT16_MAX, 0, 32768), 49151, 1);
    EXPECT_NEAR(stick(49151, UINT16_MAX, 0, 32768), 16383, 1);
}

TEST(CalibrateStick, InvertedDeadzone)
{
    EXPECT_EQ(stick(32768 + 999, UINT16_MAX, 0, 32768, 1000), kStickCentre);
    EXPECT_EQ(stick(32768 - 999, UINT16_MAX, 0, 32768, 1000), kStickCentre);
    EXPECT_EQ(stick(0, UINT16_MAX, 0, 32768, 1000), UINT16_MAX);
    EXPECT_EQ(stick(UINT16_MAX, UINT16_MAX, 0, 32768, 1000), 0);
}

TEST(CalibrateStick, InvertedOutsideRangeClamps)
{
    EXPECT_EQ(stick(1000, 62000, 2000, 32000), UINT16_MAX);
    EXPECT_EQ(stick(64000, 62000, 2000, 32000), 0);
}

TEST(AnalogTriggerPressed, JoyHighIsStrictlyAbove)
{
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyHigh, 40000, 40000, 0));
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyHigh, 40001, 40000, 0));
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyHigh, 0, 40000, 0));
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyHigh, UINT16_MAX, 40000, 0));
}

TEST(AnalogTriggerPressed, JoyLowIsStrictlyBelow)
{
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyLow, 20000, 20000, 0));
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyLow, 19999, 20000, 0));
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyLow, 0, 20000, 0));
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_JoyLow, UINT16_MAX, 20000, 0));
}

TEST(AnalogTriggerPressed, ExactOnlyMatchesTheValue)
{
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_Exact, 1234, 1234, 0));
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_Exact, 1233, 1234, 0));
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_Exact, 1235, 1234, 0));
}

TEST(AnalogTriggerPressed, RangeExcludesBothEnds)
{
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_Range, 10000, 10000, 20000));
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_Range, 10001, 10000, 20000));
    EXPECT_TRUE(analog_trigger_pressed(AnalogToDigitalTriggerType_Range, 19999, 10000, 20000));
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_Range, 20000, 10000, 20000));
}

TEST(AnalogTriggerPressed, EmptyRangeNeverPresses)
{
    EXPECT_FALSE(analog_trigger_pressed(AnalogToDigitalTriggerType_Range, 15000, 20000, 10000));
}
