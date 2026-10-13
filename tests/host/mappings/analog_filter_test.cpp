#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "mappings/analog_filter.hpp"

// AnalogFilter: the filters run on raw analog values before calibration.
// RapidTrigger: minipad style rapid trigger on calibrated travel.
namespace
{
constexpr float kStickCentre = UINT16_MAX / 2;

proto_AnalogFilter no_filters()
{
    proto_AnalogFilter config;
    memset(&config, 0, sizeof(config));
    return config;
}

proto_AnalogFilter low_pass(float cutoff)
{
    auto config = no_filters();
    config.has_lowPassCutoff = true;
    config.lowPassCutoff = cutoff;
    return config;
}

proto_AnalogFilter high_pass(float cutoff)
{
    auto config = no_filters();
    config.has_highPassCutoff = true;
    config.highPassCutoff = cutoff;
    return config;
}

proto_AnalogFilter zpd(uint32_t window)
{
    auto config = no_filters();
    config.has_zpdWindow = true;
    config.zpdWindow = window;
    return config;
}

proto_AnalogFilter responsive()
{
    auto config = no_filters();
    config.has_responsive = true;
    config.responsive = true;
    return config;
}

// Feed value every step_us for duration_us, starting after now_us, returning the last output
uint16_t hold(AnalogFilter &filter, uint64_t &now_us, uint16_t value, uint64_t duration_us, uint64_t step_us = 100)
{
    uint16_t out = 0;
    for (uint64_t t = 0; t < duration_us; t += step_us)
    {
        now_us += step_us;
        out = filter.update(value, now_us);
    }
    return out;
}

proto_RapidTrigger rapid(uint32_t down, uint32_t up, bool continuous = false)
{
    proto_RapidTrigger config;
    memset(&config, 0, sizeof(config));
    config.downSensitivity = down;
    config.upSensitivity = up;
    config.has_continuous = true;
    config.continuous = continuous;
    return config;
}
} // namespace

TEST(AnalogFilter, NothingConfiguredPassesValuesThrough)
{
    AnalogFilter filter(no_filters(), kStickCentre);
    EXPECT_FALSE(filter.enabled());
    EXPECT_EQ(filter.update(1234, 0), 1234);
    EXPECT_EQ(filter.update(60000, 1), 60000);
}

TEST(AnalogFilter, ZeroCutoffIsOff)
{
    EXPECT_FALSE(AnalogFilter(low_pass(0), 0).enabled());
    EXPECT_FALSE(AnalogFilter(high_pass(0), 0).enabled());
}

TEST(AnalogFilter, LowPassReaches63PercentAfterOneTimeConstant)
{
    // a first order low pass at 10Hz has a time constant of 1 / (2 * pi * 10) = 15.9ms
    AnalogFilter filter(low_pass(10), 0);
    uint64_t now = 1000000;
    filter.update(0, now);
    uint16_t out = hold(filter, now, 60000, 15915);
    EXPECT_NEAR(out, 60000 * 0.632, 60000 * 0.01);
    // and settles on the input
    EXPECT_NEAR(hold(filter, now, 60000, 200000), 60000, 1);
}

TEST(AnalogFilter, LowPassSmoothsNoiseFasterThanTheCutoff)
{
    AnalogFilter filter(low_pass(10), 0);
    uint64_t now = 1000000;
    filter.update(30000, now);
    uint16_t lowest = UINT16_MAX, highest = 0;
    // +-2000 alternating every 100us is 5kHz, far above the cutoff
    for (int i = 0; i < 2000; i++)
    {
        now += 100;
        uint16_t out = filter.update(i % 2 ? 32000 : 28000, now);
        lowest = std::min(lowest, out);
        highest = std::max(highest, out);
    }
    EXPECT_LT(highest - lowest, 100);
}

TEST(AnalogFilter, LowPassDoesNothingWithoutTimePassing)
{
    AnalogFilter filter(low_pass(10), 0);
    filter.update(0, 1000);
    EXPECT_EQ(filter.update(60000, 1000), 0);
}

TEST(AnalogFilter, HighPassLetsStepsThroughThenReturnsToRest)
{
    AnalogFilter filter(high_pass(1), kStickCentre);
    uint64_t now = 1000000;
    filter.update(20000, now);
    // a sudden move shows up straight away, offset onto the rest point
    now += 100;
    EXPECT_NEAR(filter.update(30000, now), kStickCentre + 10000, 10);
    // then a held value drifts back to rest, after a few time constants (0.16s at 1Hz)
    EXPECT_NEAR(hold(filter, now, 30000, 2000000), kStickCentre, 2);
}

TEST(AnalogFilter, ZeroPointDriftFollowsAnInputNearRest)
{
    AnalogFilter filter(zpd(1000), kStickCentre);
    uint64_t now = 1000000;
    filter.update(kStickCentre + 700, now);
    // the time constant is 1s, so after 10s the drift is gone
    EXPECT_NEAR(hold(filter, now, kStickCentre + 700, 10000000), kStickCentre, 1);
    // real movement past the window keeps the learned offset rather than being followed
    EXPECT_NEAR(hold(filter, now, 60000, 10000000), 60000 - 700, 1);
}

TEST(AnalogFilter, ZeroPointDriftIgnoresInputsOutsideTheWindow)
{
    AnalogFilter filter(zpd(1000), kStickCentre);
    uint64_t now = 1000000;
    filter.update(50000, now);
    EXPECT_EQ(hold(filter, now, 50000, 10000000), 50000);
}

TEST(AnalogFilter, ResponsiveSleepsThroughSmallNoise)
{
    AnalogFilter filter(responsive(), 0);
    uint64_t now = 1000000;
    filter.update(30000, now);
    uint16_t first = hold(filter, now, 30000, 100000, 1000);
    // +-1 on a 10 bit reading is +-64 here, below the activity threshold of 4 (256)
    for (int i = 0; i < 1000; i++)
    {
        now += 1000;
        EXPECT_EQ(filter.update(i % 2 ? 30064 : 29936, now), first);
    }
}

TEST(AnalogFilter, ResponsiveFollowsLargeMovesQuickly)
{
    AnalogFilter filter(responsive(), 0);
    uint64_t now = 1000000;
    filter.update(10000, now);
    hold(filter, now, 10000, 100000, 1000);
    // a large move snaps straight to the input (snap curve saturates at 1) within a few steps
    EXPECT_NEAR(hold(filter, now, 50000, 10000, 1000), 50000, 64);
}

TEST(AnalogFilter, ResponsiveStepsAt1kHz)
{
    AnalogFilter filter(responsive(), 0);
    filter.update(10000, 1000000);
    // half a step in, it hasn't moved yet
    EXPECT_EQ(filter.update(50000, 1000500), 10000);
}

TEST(RapidTrigger, PressesAtTheActuationPoint)
{
    RapidTrigger rt(rapid(2000, 2000), 20000);
    EXPECT_FALSE(rt.update(0));
    EXPECT_FALSE(rt.update(19999));
    EXPECT_TRUE(rt.update(20000));
}

TEST(RapidTrigger, ReleasesOnMovingUpAndPressesOnMovingDown)
{
    RapidTrigger rt(rapid(3000, 2000), 20000);
    rt.update(0);
    EXPECT_TRUE(rt.update(40000));
    // up by less than the up sensitivity stays pressed
    EXPECT_TRUE(rt.update(38001));
    EXPECT_FALSE(rt.update(38000));
    // the highest point while released is now 38000, going up further moves it
    EXPECT_FALSE(rt.update(35000));
    EXPECT_FALSE(rt.update(37999));
    EXPECT_TRUE(rt.update(38000));
}

TEST(RapidTrigger, PeakFollowsTheDeepestPointWhilePressed)
{
    RapidTrigger rt(rapid(2000, 2000), 20000);
    rt.update(0);
    rt.update(30000);
    EXPECT_TRUE(rt.update(60000));
    EXPECT_TRUE(rt.update(58001));
    EXPECT_FALSE(rt.update(58000));
}

TEST(RapidTrigger, LeavingTheZoneReleasesAndNeedsTheActuationPointAgain)
{
    RapidTrigger rt(rapid(2000, 2000), 20000);
    rt.update(0);
    EXPECT_TRUE(rt.update(25000));
    // back past actuation by the up sensitivity ends the zone
    EXPECT_FALSE(rt.update(18000));
    EXPECT_FALSE(rt.update(5000));
    // pressing down by the down sensitivity outside the zone does nothing
    EXPECT_FALSE(rt.update(8000));
    EXPECT_TRUE(rt.update(20000));
}

TEST(RapidTrigger, ContinuousKeepsTheZoneUntilFullyReleased)
{
    RapidTrigger rt(rapid(2000, 2000, true), 20000);
    rt.update(0);
    EXPECT_TRUE(rt.update(25000));
    EXPECT_FALSE(rt.update(5000));
    // still in the zone below the actuation point
    EXPECT_TRUE(rt.update(7000));
    // nearly fully released (within the up sensitivity of 0) ends it
    EXPECT_FALSE(rt.update(2000));
    EXPECT_FALSE(rt.update(10000));
    EXPECT_TRUE(rt.update(20000));
}

namespace
{
class ProbeAxis : public GamepadAxisMapping
{
public:
    using GamepadAxisMapping::GamepadAxisMapping;
    uint32_t value() const { return m_calibrated_value; }
};

class ProbeButton : public GamepadButtonMapping
{
public:
    using GamepadButtonMapping::GamepadButtonMapping;
    bool value() const { return m_last_value; }
};

class FilteredMapping : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();
};
} // namespace

TEST_F(FilteredMapping, AxisCalibratesTheFilteredValue)
{
    auto config = with_gamepad_axis(stick_config(), Gamepad_LeftStickX);
    config.has_filter = true;
    config.filter = low_pass(10);
    auto axis = drive<ProbeAxis>(config, profile);
    axis.set_analog(kStickCentre);
    fake_time::set_us(fake_time::now_us + 100);
    axis.set_analog(UINT16_MAX);
    // one 100us step of a 15.9ms time constant barely moves
    EXPECT_LT(axis->value(), kStickCentre + 1000);
}

TEST_F(FilteredMapping, ZeroPointDriftCentresAStick)
{
    auto config = with_gamepad_axis(stick_config(), Gamepad_LeftStickX);
    config.has_filter = true;
    config.filter = zpd(1000);
    auto axis = drive<ProbeAxis>(config, profile);
    for (int i = 0; i < 1000; i++)
    {
        fake_time::set_us(fake_time::now_us + 10000);
        axis.set_analog(kStickCentre + 500);
    }
    EXPECT_NEAR(axis->value(), kStickCentre, 1);
}

TEST_F(FilteredMapping, ButtonUsesRapidTriggerOnCalibratedTravel)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_min = config.has_max = true;
    // an inverted sensor: the raw value drops as it is pressed
    config.min = 50000;
    config.max = 10000;
    config.has_triggerValue = true;
    config.triggerValue = UINT16_MAX / 2;
    config.has_rapidTrigger = true;
    config.rapidTrigger = rapid(4000, 4000);
    auto button = drive<ProbeButton>(config, profile);
    button.set_analog(50000);
    EXPECT_FALSE(button->value());
    // half way is 30000 raw
    button.set_analog(29000);
    EXPECT_TRUE(button->value());
    button.set_analog(20000);
    EXPECT_TRUE(button->value());
    // back up 3000 raw is 4915 of travel, past the up sensitivity
    button.set_analog(23000);
    EXPECT_FALSE(button->value());
    button.set_analog(20000);
    EXPECT_TRUE(button->value());
}

TEST_F(FilteredMapping, RapidTriggerWithoutCalibrationUsesRawTravel)
{
    auto config = gamepad_button(Gamepad_A);
    config.has_triggerValue = true;
    config.triggerValue = 30000;
    config.has_rapidTrigger = true;
    config.rapidTrigger = rapid(1000, 1000);
    auto button = drive<ProbeButton>(config, profile);
    button.set_analog(0);
    EXPECT_FALSE(button->value());
    button.set_analog(30000);
    EXPECT_TRUE(button->value());
    button.set_analog(29000);
    EXPECT_FALSE(button->value());
}
