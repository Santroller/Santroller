#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "protocols/rock_band_drums.hpp"

// Drum outputs: pad / cymbal hits (calibrated like triggers, so 0 is no hit) become velocities,
// face buttons and the Rock Band pad / cymbal flags. Xbox 360 and PS3 Rock Band velocities are
// inverted, a harder hit giving a smaller value. RB4 kits (Xbox One, PS4, PS5) are not: real
// hardware sends larger values for harder hits.
namespace
{
proto_Mapping rb_drum(RockBandDrumsAxisType type)
{
    proto_Mapping config = trigger_config();
    config.mapping.which_mapping = proto_Output_rbDrumAxis_tag;
    config.mapping.mapping.rbDrumAxis = type;
    return config;
}

proto_Mapping gh_drum(GuitarHeroDrumsAxisType type)
{
    proto_Mapping config = trigger_config();
    config.mapping.which_mapping = proto_Output_ghDrumAxis_tag;
    config.mapping.mapping.ghDrumAxis = type;
    return config;
}

class Drums : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(RockBandDrums);

    template <typename T = RockBandDrumsAxisMapping>
    Driven<T> hit(const proto_Mapping &config, uint16_t value)
    {
        auto driven = drive<T>(config, profile);
        driven.set_analog(value);
        return driven;
    }

    // What the device does before building each report
    void next_report()
    {
        profile->reset_drum_state();
    }
};
} // namespace

// Guitar Hero

TEST_F(Drums, GHXInputVelocitiesAreSevenBitMidi)
{
    profile->subtype = GuitarHeroDrums;
    Report<XInputGuitarHeroDrums_Data_t> report;
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_RedPad), UINT16_MAX)->update_xinput(report.buf());
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_KickPedal), 0x8000)->update_xinput(report.buf());
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_OrangePad), 0x0200)->update_xinput(report.buf());
    EXPECT_EQ(report->redVelocity, 0x7F);
    EXPECT_TRUE(report->b);
    EXPECT_EQ(report->kickVelocity, 0x40);
    EXPECT_TRUE(report->leftShoulder);
    EXPECT_EQ(report->orangeVelocity, 1);
    EXPECT_TRUE(report->rightShoulder);
}

TEST_F(Drums, GHNoHitLeavesTheReportAlone)
{
    profile->subtype = GuitarHeroDrums;
    Report<XInputGuitarHeroDrums_Data_t> report;
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_GreenPad), 0)->update_xinput(report.buf());
    EXPECT_EQ(report->greenVelocity, 0);
    EXPECT_FALSE(report->a);
}

TEST_F(Drums, GHTwoKickPedalsWithImpulseDebounce)
{
    profile->subtype = GuitarHeroDrums;
    auto pedal1_cfg = gh_drum(GuitarHeroDrums_KickPedal);
    pedal1_cfg.has_debounce100us = true;
    pedal1_cfg.debounce100us = 20; // 2ms
    pedal1_cfg.has_debounceMode = true;
    pedal1_cfg.debounceMode = DebounceImpulse;

    auto pedal2_cfg = gh_drum(GuitarHeroDrums_KickPedal);
    pedal2_cfg.has_debounce100us = true;
    pedal2_cfg.debounce100us = 20; // 2ms
    pedal2_cfg.has_debounceMode = true;
    pedal2_cfg.debounceMode = DebounceImpulse;

    auto pedal1 = drive<GuitarHeroDrumsAxisMapping>(pedal1_cfg, profile);
    auto pedal2 = drive<GuitarHeroDrumsAxisMapping>(pedal2_cfg, profile);

    // Press pedal 1
    pedal1.set_analog(UINT16_MAX);
    pedal2.set_analog(0);
    Report<XInputGuitarHeroDrums_Data_t> report1;
    pedal1->update_xinput(report1.buf());
    pedal2->update_xinput(report1.buf());
    EXPECT_EQ(report1->kickVelocity, 0x7F);
    EXPECT_TRUE(report1->leftShoulder);

    // After 2ms, pedal 1 is still physically held down
    fake_time::advance_us(2000);
    pedal1.set_analog(UINT16_MAX);
    pedal2.set_analog(0);
    Report<XInputGuitarHeroDrums_Data_t> report2;
    pedal1->update_xinput(report2.buf());
    pedal2->update_xinput(report2.buf());
    EXPECT_EQ(report2->kickVelocity, 0);
    EXPECT_FALSE(report2->leftShoulder);

    // While pedal 1 remains held down, pedal 2 is struck!
    fake_time::advance_us(5000);
    pedal1.set_analog(UINT16_MAX);
    pedal2.set_analog(UINT16_MAX);
    Report<XInputGuitarHeroDrums_Data_t> report3;
    pedal1->update_xinput(report3.buf());
    pedal2->update_xinput(report3.buf());
    EXPECT_EQ(report3->kickVelocity, 0x7F);
    EXPECT_TRUE(report3->leftShoulder);
}

TEST_F(Drums, GHPS3Velocities)
{
    profile->subtype = GuitarHeroDrums;
    Report<PS3GuitarHeroDrums_Data_t> report;
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_YellowPad), UINT16_MAX)->update_ps3(report.buf());
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_GreenPad), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->yellowVelocity, 0x7F);
    EXPECT_EQ(report->greenVelocity, 0x40);
}

// The OG Xbox Guitar Hero drum report has the same 7 bit velocity bytes as the 360 one
TEST_F(Drums, GHOGXboxVelocitiesMatchXInput)
{
    profile->subtype = GuitarHeroDrums;
    Report<OGXboxGuitarHeroDrums_Data_t> report;
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_RedPad), 0x8000)->update_ogxbox(report.buf());
    hit<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_KickPedal), UINT16_MAX)->update_ogxbox(report.buf());
    EXPECT_EQ(report->redVelocity, 0x40);
    EXPECT_EQ(report->kickVelocity, 0x7F);
}

// Rock Band, 360 / XInput: red and blue are positive, yellow and green negative, and harder hits
// are closer to 0 (RB1 kits always send 0)

TEST_F(Drums, RBXInputPadSetsButtonFlagAndVelocity)
{
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_RedPad), UINT16_MAX)->update_xinput(report.buf());
    EXPECT_TRUE(report->b);
    EXPECT_TRUE(report->padFlag);
    EXPECT_FALSE(report->cymbalFlag);
    EXPECT_EQ(report->redVelocity, 1);
}

TEST_F(Drums, RBXInputVelocitySigns)
{
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_YellowPad), 0x8000)->update_xinput(report.buf());
    hit(rb_drum(RockBandDrums_BluePad), 0x8000)->update_xinput(report.buf());
    hit(rb_drum(RockBandDrums_GreenPad), 0x8000)->update_xinput(report.buf());
    EXPECT_EQ(report->yellowVelocity, -16384);
    EXPECT_EQ(report->blueVelocity, 16384);
    EXPECT_EQ(report->greenVelocity, -16384);
}

TEST_F(Drums, RBXInputHarderHitIsSmaller)
{
    Report<XInputRockBandDrums_Data_t> soft;
    next_report();
    hit(rb_drum(RockBandDrums_BluePad), 0x1000)->update_xinput(soft.buf());
    Report<XInputRockBandDrums_Data_t> hard;
    next_report();
    hit(rb_drum(RockBandDrums_BluePad), 0xF000)->update_xinput(hard.buf());
    EXPECT_GT(soft->blueVelocity, hard->blueVelocity);
    EXPECT_GT(hard->blueVelocity, 0);
}

// 32768 - (1 >> 1) is 32768, which doesn't fit the signed 16 bit velocity, so the softest red /
// blue hit mustn't wrap round to the wrong sign
TEST_F(Drums, RBXInputSoftestRedHitStaysPositive)
{
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_RedPad), 1)->update_xinput(report.buf());
    EXPECT_GT(report->redVelocity, 0);
}

TEST_F(Drums, RBXInputCymbalSetsCymbalFlagAndDpad)
{
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX)->update_xinput(report.buf());
    EXPECT_TRUE(report->y);
    EXPECT_TRUE(report->cymbalFlag);
    EXPECT_FALSE(report->padFlag);
    EXPECT_TRUE(report->dpadUp);

    Report<XInputRockBandDrums_Data_t> blue;
    next_report();
    hit(rb_drum(RockBandDrums_BlueCymbal), UINT16_MAX)->update_xinput(blue.buf());
    EXPECT_TRUE(blue->dpadDown);

    Report<XInputRockBandDrums_Data_t> green;
    next_report();
    hit(rb_drum(RockBandDrums_GreenCymbal), UINT16_MAX)->update_xinput(green.buf());
    EXPECT_TRUE(green->a);
    EXPECT_TRUE(green->cymbalFlag);
    EXPECT_FALSE(green->dpadUp);
    EXPECT_FALSE(green->dpadDown);
}

TEST_F(Drums, RBXInputPadAndCymbalOfOneColourUseTheRedVelocityForTheCymbal)
{
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_YellowPad), UINT16_MAX)->update_xinput(report.buf());
    hit(rb_drum(RockBandDrums_YellowCymbal), 0x8000)->update_xinput(report.buf());
    EXPECT_TRUE(report->padFlag);
    EXPECT_TRUE(report->cymbalFlag);
    EXPECT_EQ(report->yellowVelocity, -1);
    EXPECT_EQ(report->redVelocity, 16384);
    EXPECT_FALSE(report->b);
}

// Rock Band, PS3

TEST_F(Drums, RBPS3VelocitiesAreInvertedBytes)
{
    Report<PS3RockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_RedPad), UINT16_MAX)->update_ps3(report.buf());
    hit(rb_drum(RockBandDrums_GreenPad), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->redVelocity, 0x00);
    EXPECT_EQ(report->greenVelocity, 0x7F);
    EXPECT_TRUE(report->b);
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->padFlag);
}

TEST_F(Drums, RBPS3TwoCymbalsKeepTheHarderHitsDpad)
{
    // the dpad can only show one of the yellow (up) and blue (down) cymbals
    Report<PS3RockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_YellowCymbal), 0x4000)->update_ps3(report.buf());
    hit(rb_drum(RockBandDrums_BlueCymbal), 0x8000)->update_ps3(report.buf());
    EXPECT_FALSE(report->dpadUp);
    EXPECT_TRUE(report->dpadDown);

    Report<PS3RockBandDrums_Data_t> yellow;
    next_report();
    hit(rb_drum(RockBandDrums_YellowCymbal), 0x8000)->update_ps3(yellow.buf());
    hit(rb_drum(RockBandDrums_BlueCymbal), 0x8000)->update_ps3(yellow.buf());
    EXPECT_TRUE(yellow->dpadUp);
    EXPECT_FALSE(yellow->dpadDown);
}

// Rock Band 4

TEST_F(Drums, RBPS4HasSeparateFullByteCymbalVelocities)
{
    Report<PS4RockBandDrums_Data_t> report;
    hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX)->update_ps4(report.buf());
    hit(rb_drum(RockBandDrums_YellowPad), 0x8000)->update_ps4(report.buf());
    hit(rb_drum(RockBandDrums_GreenPad), 0x00FF)->update_ps4(report.buf());
    EXPECT_EQ(report->yellowCymbalVelocity, 0xFF);
    EXPECT_EQ(report->yellowVelocity, 0x80);
    EXPECT_EQ(report->greenVelocity, 1);
    EXPECT_TRUE(report->y);
}

TEST_F(Drums, RBPS5VelocitiesUseTheFullByte)
{
    Report<PS5RockBandDrums_Data_t> report;
    hit(rb_drum(RockBandDrums_RedPad), UINT16_MAX)->update_ps5(report.buf());
    EXPECT_EQ(report->redVelocity, 0xFF);
    EXPECT_TRUE(report->b);
}

TEST_F(Drums, RBXboxOneVelocitiesStopAtSevenAndAreNeverZero)
{
    Report<XboxOneRockBandDrums_Data_t> report;
    hit(rb_drum(RockBandDrums_RedPad), UINT16_MAX)->update_xboxone(report.buf());
    hit(rb_drum(RockBandDrums_GreenCymbal), 0x0FFF)->update_xboxone(report.buf());
    hit(rb_drum(RockBandDrums_BluePad), 0x8000)->update_xboxone(report.buf());
    EXPECT_EQ(report->redVelocity, 7);
    EXPECT_EQ(report->greenCymbalVelocity, 1);
    EXPECT_EQ(report->blueVelocity, 4);
    EXPECT_TRUE(report->b);
    EXPECT_TRUE(report->a);
    EXPECT_TRUE(report->x);
}

// A hosted kit's velocity has to come back out unchanged, and anything past the Xbox One stock range is clamped
TEST(DrumsVelocity, RB4HostVelocitiesRoundTrip)
{
    for (uint8_t max : {(uint8_t)RB4_XBOX_ONE_MAX_VELOCITY, (uint8_t)RB4_PS_MAX_VELOCITY})
    {
        for (int velocity = 1; velocity <= max; velocity++)
        {
            EXPECT_EQ(rb4_drums_axis_to_velocity(rb4_drums_velocity_to_axis(velocity, max), max), velocity);
        }
        EXPECT_EQ(rb4_drums_velocity_to_axis(0, max), 0);
        EXPECT_EQ(rb4_drums_velocity_to_axis(max, max), UINT16_MAX);
    }
    EXPECT_EQ(rb4_drums_velocity_to_axis(0xF, RB4_XBOX_ONE_MAX_VELOCITY), UINT16_MAX);
}

TEST_F(Drums, RBOGXboxVelocities)
{
    Report<OGXboxRockBandDrums_Data_t> report;
    hit(rb_drum(RockBandDrums_BlueCymbal), UINT16_MAX)->update_ogxbox(report.buf());
    EXPECT_EQ(report->blueVelocity, INT16_MAX);
    EXPECT_TRUE(report->x);
    EXPECT_TRUE(report->cymbalFlag);
    EXPECT_TRUE(report->dpadDown);
}

TEST_F(Drums, PowerGigPS3Pressure)
{
    profile->subtype = PowerGigDrum;
    Report<PS3PowerGigDrums_Data_t> report;
    hit<PowerGigDrumsAxisMapping>(rb_drum(RockBandDrums_BluePad), 0x8000)->update_ps3(report.buf());
    EXPECT_EQ(report->pressure_blue, 0x80);
    EXPECT_TRUE(report->left_blue);
}

// Cymbal glitch fix: Rock Band can't take two cymbals at once, or the green pad and cymbal
// together, so the second hit waits until the first has been off for a debounce (25ms default)

class CymbalGlitch : public Drums
{
protected:
    void SetUp() override
    {
        Drums::SetUp();
        profile->cymbal_glitch_fix = true;
    }
};

TEST_F(Drums, WithoutTheFixTwoCymbalsGoTogether)
{
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX)->update_xinput(report.buf());
    hit(rb_drum(RockBandDrums_BlueCymbal), UINT16_MAX)->update_xinput(report.buf());
    EXPECT_TRUE(report->y);
    EXPECT_TRUE(report->x);
}

TEST_F(CymbalGlitch, SecondCymbalWaitsForTheFirst)
{
    auto yellow = hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX);
    auto blue = hit(rb_drum(RockBandDrums_BlueCymbal), UINT16_MAX);
    Report<XInputRockBandDrums_Data_t> first;
    next_report();
    yellow->update_xinput(first.buf());
    blue->update_xinput(first.buf());
    EXPECT_TRUE(first->y);
    EXPECT_FALSE(first->x);
}

TEST_F(CymbalGlitch, SecondCymbalGoesADebounceAfterTheFirstEnds)
{
    auto yellow = hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX);
    auto blue = hit(rb_drum(RockBandDrums_BlueCymbal), UINT16_MAX);
    Report<XInputRockBandDrums_Data_t> first;
    next_report();
    yellow->update_xinput(first.buf());

    // yellow released: the next report has no cymbal, which ends it
    yellow.set_analog(0);
    Report<XInputRockBandDrums_Data_t> gap;
    next_report();
    yellow->update_xinput(gap.buf());
    Report<XInputRockBandDrums_Data_t> blocked;
    next_report();
    blue->update_xinput(blocked.buf());
    EXPECT_FALSE(blocked->x);

    fake_time::advance_us(25001);
    Report<XInputRockBandDrums_Data_t> allowed;
    next_report();
    blue->update_xinput(allowed.buf());
    EXPECT_TRUE(allowed->x);
}

TEST_F(CymbalGlitch, GreenPadWaitsForGreenCymbal)
{
    auto cymbal = hit(rb_drum(RockBandDrums_GreenCymbal), UINT16_MAX);
    auto pad = hit(rb_drum(RockBandDrums_GreenPad), UINT16_MAX);
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    cymbal->update_xinput(report.buf());
    pad->update_xinput(report.buf());
    EXPECT_TRUE(report->cymbalFlag);
    EXPECT_FALSE(report->padFlag);
}

TEST_F(CymbalGlitch, OtherPadsAreNeverHeldBack)
{
    auto cymbal = hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX);
    auto pad = hit(rb_drum(RockBandDrums_RedPad), UINT16_MAX);
    Report<XInputRockBandDrums_Data_t> report;
    next_report();
    cymbal->update_xinput(report.buf());
    pad->update_xinput(report.buf());
    EXPECT_TRUE(report->y);
    EXPECT_TRUE(report->b);
}

TEST_F(CymbalGlitch, ConfiguredDebounceSetsTheGap)
{
    auto blue_config = rb_drum(RockBandDrums_BlueCymbal);
    blue_config.has_debounce = true;
    blue_config.debounce = 5; // ms
    auto yellow = hit(rb_drum(RockBandDrums_YellowCymbal), UINT16_MAX);
    auto blue = hit(blue_config, UINT16_MAX);
    Report<XInputRockBandDrums_Data_t> first;
    next_report();
    yellow->update_xinput(first.buf());

    // yellow released, and a report goes out without it, which ends it
    yellow.set_analog(0);
    next_report();
    next_report();

    fake_time::advance_us(5000);
    Report<XInputRockBandDrums_Data_t> blocked;
    next_report();
    blue->update_xinput(blocked.buf());
    EXPECT_FALSE(blocked->x);

    fake_time::advance_us(1);
    Report<XInputRockBandDrums_Data_t> allowed;
    next_report();
    blue->update_xinput(allowed.buf());
    EXPECT_TRUE(allowed->x);
}
