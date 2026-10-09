#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "managers/config_manager.hpp"
#include "protocols/rb_pickup.hpp"

// Guitar and turntable outputs: whammy (calibrated like a trigger, resting at 0), tilt
// (calibrated like a stick, resting at the centre), the Rock Band pickup selector and the
// DJ Hero turntable axes
namespace
{
proto_Mapping gh_axis(GuitarHeroGuitarAxisType type)
{
    proto_Mapping config = type == GuitarHeroGuitar_Whammy ? trigger_config() : stick_config();
    config.mapping.which_mapping = proto_Output_ghAxis_tag;
    config.mapping.mapping.ghAxis = type;
    return config;
}

proto_Mapping rb_axis(RockBandGuitarAxisType type)
{
    proto_Mapping config = type == RockBandGuitar_Tilt ? stick_config() : trigger_config();
    config.mapping.which_mapping = proto_Output_rbAxis_tag;
    config.mapping.mapping.rbAxis = type;
    return config;
}

proto_Mapping ghl_axis(GuitarHeroLiveGuitarAxisType type)
{
    proto_Mapping config = type == GuitarHeroLiveGuitar_Whammy ? trigger_config() : stick_config();
    config.mapping.which_mapping = proto_Output_ghlAxis_tag;
    config.mapping.mapping.ghlAxis = type;
    return config;
}

proto_Mapping ghl_button(GuitarHeroLiveGuitarButtonType type)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_ghlButton_tag;
    config.mapping.mapping.ghlButton = type;
    return config;
}

proto_Mapping gh_button(GuitarHeroGuitarButtonType type)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_ghButton_tag;
    config.mapping.mapping.ghButton = type;
    return config;
}

proto_Mapping rb_button(RockBandGuitarButtonType type)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_rbButton_tag;
    config.mapping.mapping.rbButton = type;
    return config;
}

proto_Mapping djh_axis(DJHTurntableAxisType type)
{
    proto_Mapping config = stick_config();
    config.mapping.which_mapping = proto_Output_djhAxis_tag;
    config.mapping.mapping.djhAxis = type;
    return config;
}

class Guitar : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(GuitarHeroGuitar);

    template <typename T>
    Driven<T> at(const proto_Mapping &config, uint16_t value)
    {
        auto driven = drive<T>(config, profile);
        driven.set_analog(value);
        return driven;
    }

    template <typename T>
    Driven<T> pressed(const proto_Mapping &config, bool value = true)
    {
        auto driven = drive<T>(config, profile);
        driven.set_digital(value);
        return driven;
    }
};

class Turntable : public Guitar
{
protected:
    void SetUp() override
    {
        Guitar::SetUp();
        profile->subtype = DjHeroTurntable;
        ConfigManager::instance().set_current_mode(ModeXbox360);
    }
    void TearDown() override
    {
        ConfigManager::instance().set_current_mode(ModeHid);
    }
};
} // namespace

// Guitar Hero

TEST_F(Guitar, GHXInputWhammyRestsAtTheBottomOfTheAxis)
{
    Report<XInputGuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), UINT16_MAX)->update_xinput(report.buf());
    EXPECT_EQ(report->whammy, INT16_MAX);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 1)->update_xinput(report.buf());
    EXPECT_EQ(report->whammy, INT16_MIN + 1);
}

TEST_F(Guitar, GHWhammyAtRestIsLeftAlone)
{
    Report<XInputGuitarHeroGuitar_Data_t> report;
    report->whammy = 99;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->whammy, 99);
}

TEST_F(Guitar, GHXInputTiltIsSigned)
{
    Report<XInputGuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), UINT16_MAX)->update_xinput(report.buf());
    EXPECT_EQ(report->tilt, INT16_MAX);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->tilt, INT16_MIN);
}

TEST_F(Guitar, GHHidMatchesXInput)
{
    Report<XInputGuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), UINT16_MAX)->update_hid(report.buf());
    EXPECT_EQ(report->whammy, INT16_MAX);
}

TEST_F(Guitar, GHPS3Whammy)
{
    Report<PS3GuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), UINT16_MAX)->update_ps3(report.buf());
    EXPECT_EQ(report->whammy, 0xFF);
}

TEST_F(Guitar, GHPS3TiltPeaksAtTheAccelerometerCentre)
{
    // PS3 guitars report tilt on the 10 bit accelerometer, which sits around 0x180 level and
    // reaches 0x200 tilted up
    Report<PS3GuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), UINT16_MAX)->update_ps3(report.buf());
    EXPECT_EQ(report->tilt, 0x200);
    Report<PS3GuitarHeroGuitar_Data_t> level;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 32768)->update_ps3(level.buf());
    EXPECT_NEAR(level->tilt, 0x180, 1);
    EXPECT_LT(level->tilt, report->tilt);
}

TEST_F(Guitar, GHPS2WhammyCountsDownFrom7F)
{
    Report<PS2GuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), UINT16_MAX)->update_ps2(report.buf());
    EXPECT_EQ(report->whammy, 0x00);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 0x8000)->update_ps2(report.buf());
    EXPECT_EQ(report->whammy, 0x3F);
}

TEST_F(Guitar, GHPS2TiltIsDigitalNearTheEnd)
{
    Report<PS2GuitarHeroGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 60000)->update_ps2(report.buf());
    EXPECT_FALSE(report->tilt);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 60001)->update_ps2(report.buf());
    EXPECT_TRUE(report->tilt);
}

TEST_F(Guitar, GHPS4TiltIsDistanceFromLevel)
{
    Report<PS4RockBandGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), UINT16_MAX)->update_ps4(report.buf());
    EXPECT_EQ(report->tilt, 0xFF);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 32768 + 128)->update_ps4(report.buf());
    EXPECT_EQ(report->tilt, 1);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 1)->update_ps4(report.buf());
    EXPECT_EQ(report->tilt, 0xFF);
}

// abs(0 - 32768) >> 7 is 256, which mustn't wrap to 0 in the 8 bit tilt
TEST_F(Guitar, GHPS4TiltFullyDownIsFullTilt)
{
    Report<PS4RockBandGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 0)->update_ps4(report.buf());
    EXPECT_EQ(report->tilt, 0xFF);
    Report<PS5RockBandGuitar_Data_t> ps5;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 0)->update_ps5(ps5.buf());
    EXPECT_EQ(ps5->tilt, 0xFF);
}

TEST_F(Guitar, GHXboxOneWhammyAndTilt)
{
    Report<XboxOneRockBandGuitar_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 0x8000)->update_xboxone(report.buf());
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), UINT16_MAX)->update_xboxone(report.buf());
    EXPECT_EQ(report->whammy, 0x80);
    EXPECT_EQ(report->tilt, 0xFF);
}

TEST_F(Guitar, GHWiiWhammyIsFiveBit)
{
    Report<WiiGuitarDataFormat3_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), UINT16_MAX)->update_wii(3, report.buf());
    EXPECT_EQ(report->whammy, 0x1F);
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 0x8000)->update_wii(3, report.buf());
    EXPECT_EQ(report->whammy, 0x10);
}

// The Switch guitar layer only has one bit each for whammy and tilt, set past the same > 60000
// threshold as the other one bit triggers / tilts
TEST_F(Guitar, GHSwitchWhammyAndTiltPressNearFull)
{
    Report<SwitchFestivalProGuitarLayer_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 0xFE00)->update_switch(report.buf());
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Tilt), 0xFE00)->update_switch(report.buf());
    EXPECT_TRUE(report->whammy);
    EXPECT_TRUE(report->tilt);
}

TEST_F(Guitar, GHSwitchWhammyBarelyPressedIsOff)
{
    Report<SwitchFestivalProGuitarLayer_Data_t> report;
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), 0x0100)->update_switch(report.buf());
    EXPECT_FALSE(report->whammy);
}

TEST_F(Guitar, GHPS2ButtonsAlwaysHoldDpadLeft)
{
    // PS2 guitars identify themselves by holding dpad left
    Report<PS2GuitarHeroGuitar_Data_t> report;
    pressed<GuitarHeroGuitarButtonMapping>(gh_button(GuitarHeroGuitar_Green), false)->update_ps2(report.buf());
    EXPECT_TRUE(report->dpadLeft);
    EXPECT_FALSE(report->green);
}

TEST_F(Guitar, GHFretsOnXInput)
{
    Report<XInputGuitarHeroGuitar_Data_t> report;
    pressed<GuitarHeroGuitarButtonMapping>(gh_button(GuitarHeroGuitar_Green))->update_xinput(report.buf());
    pressed<GuitarHeroGuitarButtonMapping>(gh_button(GuitarHeroGuitar_Orange))->update_xinput(report.buf());
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->leftShoulder);
    EXPECT_FALSE(report->b);
}

// Guitar Hero Live

TEST_F(Guitar, GHLPS3WhammyAndTilt)
{
    Report<PS3GHLGuitar_Data_t> report;
    at<LiveGuitarAxisMapping>(ghl_axis(GuitarHeroLiveGuitar_Whammy), UINT16_MAX)->update_ps3(report.buf());
    at<LiveGuitarAxisMapping>(ghl_axis(GuitarHeroLiveGuitar_Tilt), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->whammy, 0xFF);
    EXPECT_EQ(report->tilt, 0x80);
}

TEST_F(Guitar, GHLXInputWhammyAndTilt)
{
    Report<XInputGHLGuitar_Data_t> report;
    at<LiveGuitarAxisMapping>(ghl_axis(GuitarHeroLiveGuitar_Whammy), UINT16_MAX)->update_xinput(report.buf());
    at<LiveGuitarAxisMapping>(ghl_axis(GuitarHeroLiveGuitar_Tilt), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->whammy, INT16_MAX);
    EXPECT_EQ(report->tilt, INT16_MIN);
}

// The Xbox One GHL report carries the PS3 GHL report, so whammy and tilt match it
TEST_F(Guitar, GHLXboxOneWhammyAndTiltMatchPS3)
{
    Report<XboxOneGHLGuitar_Data_t> report;
    at<LiveGuitarAxisMapping>(ghl_axis(GuitarHeroLiveGuitar_Whammy), UINT16_MAX)->update_xboxone(report.buf());
    at<LiveGuitarAxisMapping>(ghl_axis(GuitarHeroLiveGuitar_Tilt), 0x8000)->update_xboxone(report.buf());
    EXPECT_EQ(report->report.whammy, 0xFF);
    EXPECT_EQ(report->report.tilt, 0x80);
}

TEST_F(Guitar, GHLStrumSetsTheStrumBar)
{
    Report<PS3GHLGuitar_Data_t> up;
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumUp))->update_ps3(up.buf());
    EXPECT_TRUE(up->dpadUp);
    EXPECT_EQ(up->strumBar, 0x00);
    Report<PS3GHLGuitar_Data_t> down;
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumDown))->update_ps3(down.buf());
    EXPECT_TRUE(down->dpadDown);
    EXPECT_EQ(down->strumBar, 0xFF);

    Report<XInputGHLGuitar_Data_t> xinput;
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumUp))->update_xinput(xinput.buf());
    EXPECT_EQ(xinput->strumBar, INT16_MAX);
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumDown))->update_xinput(xinput.buf());
    EXPECT_EQ(xinput->strumBar, INT16_MIN);
}

// A strum mapping that isn't pressed leaves the strum bar alone, so the other direction can set it
TEST_F(Guitar, GHLReleasedStrumLeavesTheStrumBarAlone)
{
    Report<PS3GHLGuitar_Data_t> report;
    report->strumBar = 0x80;
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumUp), false)->update_ps3(report.buf());
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumDown), false)->update_ps3(report.buf());
    EXPECT_EQ(report->strumBar, 0x80);

    Report<XInputGHLGuitar_Data_t> xinput;
    pressed<LiveGuitarButtonMapping>(ghl_button(GuitarHeroLiveGuitar_StrumDown), false)->update_xinput(xinput.buf());
    EXPECT_EQ(xinput->strumBar, 0);
}

// Rock Band

TEST_F(Guitar, RBXInputWhammyAndTilt)
{
    Report<XInputRockBandGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Whammy), UINT16_MAX)->update_xinput(report.buf());
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Tilt), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->whammy, INT16_MAX);
    EXPECT_EQ(report->tilt, INT16_MIN);
}

TEST_F(Guitar, RBPickupEqualBands)
{
    // without thresholds the range is split into five equal notches
    struct
    {
        uint16_t value;
        uint8_t notch;
    } cases[] = {{0, 0}, {13107, 0}, {13108, 1}, {26214, 1}, {26215, 2}, {39321, 2}, {39322, 3}, {52428, 3}, {52429, 4}, {UINT16_MAX, 4}};
    for (auto c : cases)
    {
        Report<PS4RockBandGuitar_Data_t> report;
        at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), c.value)->update_ps4(report.buf());
        EXPECT_EQ(report->pickup, c.notch) << "value " << c.value;
    }
}

TEST_F(Guitar, RBPickupThresholds)
{
    auto config = rb_axis(RockBandGuitar_Pickup);
    config.pickupThresholds_count = 4;
    config.pickupThresholds[0] = 1000;
    config.pickupThresholds[1] = 2000;
    config.pickupThresholds[2] = 50000;
    config.pickupThresholds[3] = 60000;
    struct
    {
        uint16_t value;
        uint8_t notch;
    } cases[] = {{0, 0}, {1000, 0}, {1001, 1}, {2000, 1}, {2001, 2}, {50000, 2}, {50001, 3}, {60000, 3}, {60001, 4}, {UINT16_MAX, 4}};
    for (auto c : cases)
    {
        Report<PS4RockBandGuitar_Data_t> report;
        at<RockBandGuitarAxisMapping>(config, c.value)->update_ps4(report.buf());
        EXPECT_EQ(report->pickup, c.notch) << "value " << c.value;
    }
}

TEST_F(Guitar, RBPickupThresholdsNeedAllFour)
{
    auto config = rb_axis(RockBandGuitar_Pickup);
    config.pickupThresholds_count = 3;
    config.pickupThresholds[0] = 1000;
    Report<PS4RockBandGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(config, 2000)->update_ps4(report.buf());
    EXPECT_EQ(report->pickup, 0);
}

TEST_F(Guitar, RBPickupUsesEachConsolesValues)
{
    Report<PS3RockBandGuitar_Data_t> ps3;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), UINT16_MAX)->update_ps3(ps3.buf());
    EXPECT_EQ(ps3->pickup, 0xE5);
    Report<XInputRockBandGuitar_Data_t> xinput;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), 30000)->update_xinput(xinput.buf());
    EXPECT_EQ(xinput->pickup, 0x96);
    Report<XboxOneRockBandGuitar_Data_t> xb1;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), 20000)->update_xboxone(xb1.buf());
    EXPECT_EQ(xb1->pickup, 0x10);
    Report<PS5RockBandGuitar_Data_t> ps5;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), 45000)->update_ps5(ps5.buf());
    EXPECT_EQ(ps5->pickup, 3);
}

TEST_F(Guitar, RBPickupFirstNotchIsReportedAtRest)
{
    // the lowest notch is a real position, so it's written even though the axis is at rest
    Report<PS3RockBandGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), 0)->update_ps3(report.buf());
    EXPECT_EQ(report->pickup, 0x19);
}

TEST_F(Guitar, RBPS4Whammy)
{
    Report<PS4RockBandGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Whammy), 0x8000)->update_ps4(report.buf());
    EXPECT_EQ(report->whammy, 0x80);
}

// The Wii guitar whammy is 5 bits, like the Guitar Hero mapping
TEST_F(Guitar, RBWiiWhammyIsFiveBit)
{
    Report<WiiGuitarDataFormat3_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Whammy), 0x8000)->update_wii(3, report.buf());
    EXPECT_EQ(report->whammy, 0x10);
}

// PS3 Rock Band tilt is one bit (r1), set past the > 60000 tilt threshold
TEST_F(Guitar, RBPS3TiltIsDigital)
{
    Report<PS3RockBandGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Tilt), 0xFE00)->update_ps3(report.buf());
    EXPECT_TRUE(report->tilt);
    Report<PS3RockBandGuitar_Data_t> slight;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Tilt), 0x8100)->update_ps3(slight.buf());
    EXPECT_FALSE(slight->tilt);
}

// Same with the one bit PS2 guitar tilt, which the Guitar Hero mapping also sets past 60000
TEST_F(Guitar, RBPS2TiltIsDigital)
{
    Report<PS2GuitarHeroGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Tilt), 0xFE00)->update_ps2(report.buf());
    EXPECT_TRUE(report->tilt);
}

// The OG Xbox Rock Band guitar has signed 16 bit whammy and tilt like the 360 one
TEST_F(Guitar, RBOGXboxWhammyAndTiltAreFullRange)
{
    Report<OGXboxRockBandGuitar_Data_t> report;
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Whammy), UINT16_MAX)->update_ogxbox(report.buf());
    at<RockBandGuitarAxisMapping>(rb_axis(RockBandGuitar_Tilt), 0)->update_ogxbox(report.buf());
    EXPECT_EQ(report->whammy, INT16_MAX);
    EXPECT_EQ(report->tilt, INT16_MIN);
}

TEST_F(Guitar, RBSoloFretsSetTheFretAndSoloFlag)
{
    profile->subtype = RockBandGuitar;
    Report<XInputRockBandGuitar_Data_t> report;
    pressed<RockBandGuitarButtonMapping>(rb_button(RockBandGuitar_SoloBlue))->update_xinput(report.buf());
    EXPECT_TRUE(report->x);
    EXPECT_TRUE(report->solo);
}

TEST_F(Guitar, PowerGigPS3)
{
    Report<PS3PowerGigGuitar_Data_t> report;
    at<PowerGigGuitarAxisMapping>(rb_axis(RockBandGuitar_Whammy), UINT16_MAX)->update_ps3(report.buf());
    at<PowerGigGuitarAxisMapping>(rb_axis(RockBandGuitar_Pickup), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->whammy, 0xFF);
    EXPECT_EQ(report->pickup, 0x80);
}

// DJ Hero turntable

TEST_F(Turntable, XInputTableVelocityIsScaledDownOn360)
{
    // real 360 turntables only use a small range of the axis for the tables
    Report<XInputDJHTurntable_Data_t> report;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), UINT16_MAX)->update_xinput(report.buf());
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_RightVelocity), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->leftTableVelocity, 32767 / XINPUT_TURNTABLE_VELOCITY_SCALE);
    EXPECT_EQ(report->rightTableVelocity, -32768 / XINPUT_TURNTABLE_VELOCITY_SCALE);
}

TEST_F(Turntable, XInputKnobAndCrossfaderAreFullRange)
{
    Report<XInputDJHTurntable_Data_t> report;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_EffectsKnob), UINT16_MAX)->update_xinput(report.buf());
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_Crossfader), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->effectsKnob, INT16_MAX);
    EXPECT_EQ(report->crossfader, INT16_MIN);
}

TEST_F(Turntable, FullRangeOnPCOnlyOutsideThe360)
{
    profile->full_range_turntable_on_pc = true;
    Report<XInputDJHTurntable_Data_t> on360;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), UINT16_MAX)->update_xinput(on360.buf());
    EXPECT_EQ(on360->leftTableVelocity, 32767 / XINPUT_TURNTABLE_VELOCITY_SCALE);

    ConfigManager::instance().set_current_mode(ModeHid);
    Report<XInputDJHTurntable_Data_t> pc;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), UINT16_MAX)->update_xinput(pc.buf());
    EXPECT_EQ(pc->leftTableVelocity, INT16_MAX);
}

TEST_F(Turntable, HidIsAlwaysFullRange)
{
    Report<XInputDJHTurntable_Data_t> report;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), 0)->update_hid(report.buf());
    EXPECT_EQ(report->leftTableVelocity, INT16_MIN);
}

TEST_F(Turntable, PS3Axes)
{
    Report<PS3DJHTurntable_Data_t> report;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), UINT16_MAX)->update_ps3(report.buf());
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_EffectsKnob), UINT16_MAX)->update_ps3(report.buf());
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_Crossfader), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->leftTableVelocity, 0xFF);
    EXPECT_EQ(report->effectsKnob, 0x3FF);
    EXPECT_EQ(report->crossfader, 0x200);
}

TEST_F(Turntable, WiiVelocitiesAreSignedSixBit)
{
    Report<WiiTurntableDataFormat3_t> fast;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), UINT16_MAX)->update_wii(3, fast.buf());
    WiiTurntableIntermediateFormat3_t left;
    left.leftTableVelocity40 = fast->leftTableVelocity40;
    left.leftTableVelocity5 = fast->leftTableVelocity5;
    EXPECT_EQ(left.leftTableVelocity, 31);

    Report<WiiTurntableDataFormat3_t> back;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_RightVelocity), 0)->update_wii(3, back.buf());
    WiiTurntableIntermediateFormat3_t right;
    right.rightTableVelocity0 = back->rightTableVelocity0;
    right.rightTableVelocity21 = back->rightTableVelocity21;
    right.rightTableVelocity43 = back->rightTableVelocity43;
    right.rightTableVelocity5 = back->rightTableVelocity5;
    EXPECT_EQ(right.rightTableVelocity, -32);
}

TEST_F(Turntable, WiiKnobIsFiveBitAndCrossfaderFourBit)
{
    Report<WiiTurntableDataFormat3_t> report;
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_EffectsKnob), UINT16_MAX)->update_wii(3, report.buf());
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_Crossfader), UINT16_MAX)->update_wii(3, report.buf());
    EXPECT_EQ(report->effectsKnob20, 0x7);
    EXPECT_EQ(report->effectsKnob43, 0x3);
    EXPECT_EQ(report->crossfader, 0xF);
}

// Rock Band Pro Guitar

namespace
{
proto_Mapping pro_axis(ProGuitarAxisType type)
{
    proto_Mapping config = type == ProGuitar_Tilt ? stick_config() : trigger_config();
    config.mapping.which_mapping = proto_Output_proAxis_tag;
    config.mapping.mapping.proAxis = type;
    return config;
}
} // namespace

TEST_F(Guitar, ProGuitarFretsAreFiveBit)
{
    Report<PS3RockBandProGuitar_Data_t> report;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_LowEFret), UINT16_MAX)->update_ps3(report.buf());
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_HighEFret), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->lowEFret, 31);
    EXPECT_EQ(report->highEFret, 16);
    EXPECT_EQ(report->aFret, 0);
}

TEST_F(Guitar, ProGuitarVelocitiesAreSevenBit)
{
    Report<XInputRockBandProGuitar_Data_t> report;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_GFretVelocity), UINT16_MAX)->update_xinput(report.buf());
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_BFretVelocity), 0x8000)->update_xinput(report.buf());
    EXPECT_EQ(report->gFretVelocity, 0x7F);
    EXPECT_EQ(report->bFretVelocity, 0x40);
}

TEST_F(Guitar, ProGuitarTiltIsSevenBitAndAlwaysReported)
{
    // level is the middle of the 7 bit range, and it's written even at rest
    Report<PS3RockBandProGuitar_Data_t> level;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_Tilt), UINT16_MAX / 2)->update_ps3(level.buf());
    EXPECT_EQ(level->tilt, 0x40);
    Report<PS3RockBandProGuitar_Data_t> up;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_Tilt), UINT16_MAX)->update_ps3(up.buf());
    EXPECT_EQ(up->tilt, 0x7F);
    Report<PS3RockBandProGuitar_Data_t> down;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_Tilt), 0)->update_ps3(down.buf());
    EXPECT_EQ(down->tilt, 0);
}

TEST_F(Guitar, ProGuitarTiltRoundsToNearest)
{
    // 0x1FF is just under one 7 bit step (0x200), so it rounds up
    Report<PS3RockBandProGuitar_Data_t> report;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_Tilt), 0x1FF)->update_ps3(report.buf());
    EXPECT_EQ(report->tilt, 1);
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_Tilt), 0xFF)->update_ps3(report.buf());
    EXPECT_EQ(report->tilt, 0);
}

TEST_F(Guitar, ProGuitarTiltAlsoFillsTheAutoCalibrationBytes)
{
    Report<PS3RockBandProGuitar_Data_t> report;
    at<ProGuitarAxisMapping>(pro_axis(ProGuitar_Tilt), UINT16_MAX)->update_ps3(report.buf());
    EXPECT_EQ(report->autoCal_Light, report->tilt);
    EXPECT_EQ(report->autoCal_Microphone, report->tilt);
}
