// Decoding PS3 (and Wii USB) drum kits in protocols/ps3_drums.hpp. Reports are built byte by byte
// from the PS3 instrument report (PlasticBand Instruments/4-Lane Drums/PS3 and Wii.md and
// Instruments/5-Lane Drums/PS3.md):
//   byte 0: square (blue), cross (green), circle (red), triangle (yellow), L1 (kick 1), R1 (kick 2 /
//           GH orange), L2, R2
//   byte 1: select, start, L3 (pad flag), R3 (cymbal flag), PS
//   byte 2: hat switch, 0 is up going clockwise, 8 is centred
//   bytes 11 - 16: pressure for triangle, circle, cross, square, L1, R1, which the kits use for
//           velocity. RB velocities are inverted (RB1 kits send 0), GH ones are 7 bit MIDI velocities.
#include <gtest/gtest.h>
#include <algorithm>
#include "protocols/ps3_drums.hpp"

namespace
{
enum : uint8_t
{
    Blue = 0x01,
    Green = 0x02,
    Red = 0x04,
    Yellow = 0x08,
    Kick1 = 0x10,
    Kick2 = 0x20,
};
enum : uint8_t
{
    Start = 0x02,
    PadFlag = 0x04,
    CymbalFlag = 0x08,
};
enum : uint8_t
{
    HatUp = 0,
    HatUpRight = 1,
    HatRight = 2,
    HatDown = 4,
    HatDownLeft = 5,
    HatUpLeft = 7,
    HatCentre = 8,
};

struct Report
{
    uint8_t buf[27] = {};
    Report(uint8_t b0, uint8_t b1, uint8_t hat = HatCentre)
    {
        buf[0] = b0;
        buf[1] = b1;
        buf[2] = hat;
        buf[3] = buf[4] = buf[5] = buf[6] = 0x80;
    }
    Report &yellow(uint8_t v) { buf[11] = v; return *this; }
    Report &red(uint8_t v) { buf[12] = v; return *this; }
    Report &green(uint8_t v) { buf[13] = v; return *this; }
    Report &blue(uint8_t v) { buf[14] = v; return *this; }
    Report &kick(uint8_t v) { buf[15] = v; return *this; }
    Report &orange(uint8_t v) { buf[16] = v; return *this; }
};

const RockBandDrumsAxisType all_rb_axes[] = {
    RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad,
    RockBandDrums_YellowCymbal, RockBandDrums_BlueCymbal, RockBandDrums_GreenCymbal};

uint16_t rb(const Report &r, RockBandDrumsAxisType axis, bool rb2 = true)
{
    return rb_drum_hit(ps3_decode_rb_drums(r.buf, rb2), axis);
}

// Every axis apart from the ones listed reads 0
void expect_only(const Report &r, bool rb2, std::initializer_list<RockBandDrumsAxisType> hit)
{
    for (auto axis : all_rb_axes)
    {
        bool expect_hit = std::find(hit.begin(), hit.end(), axis) != hit.end();
        if (expect_hit)
            EXPECT_NE(rb(r, axis, rb2), 0) << axis;
        else
            EXPECT_EQ(rb(r, axis, rb2), 0) << axis;
    }
}
} // namespace

TEST(PS3Drums, Rb1PadsAreFaceButtonsWithoutVelocity)
{
    Report r(Red | Yellow | Blue | Green, 0);
    for (auto axis : {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad})
        EXPECT_EQ(rb(r, axis, false), UINT16_MAX) << axis;
    expect_only(r, false, {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad});
    expect_only(Report(0, 0), false, {});
}

TEST(PS3Drums, Rb2FaceButtonsWithoutAFlagAreNotHits)
{
    expect_only(Report(Red | Green, 0, HatUp), true, {});
    EXPECT_FALSE(ps3_rb_drums_hit_flagged(Report(Red | Green, Start, HatUp).buf));
}

TEST(PS3Drums, Rb2PadVelocitiesAreInverted)
{
    Report r = Report(Red | Yellow | Blue | Green, PadFlag).red(0x00).yellow(0x40).blue(0x80).green(0xFE);
    EXPECT_EQ(rb(r, RockBandDrums_RedPad), 0xFFFF);
    EXPECT_EQ(rb(r, RockBandDrums_YellowPad), 0xBFBF);
    EXPECT_EQ(rb(r, RockBandDrums_BluePad), 0x7F7F);
    EXPECT_EQ(rb(r, RockBandDrums_GreenPad), 0x0101);
    expect_only(r, true, {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad});
    EXPECT_TRUE(ps3_rb_drums_hit_flagged(r.buf));
}

TEST(PS3Drums, Rb2CymbalColourComesFromTheHat)
{
    expect_only(Report(Yellow, CymbalFlag, HatUp).yellow(0x10), true, {RockBandDrums_YellowCymbal});
    expect_only(Report(Blue, CymbalFlag, HatDown).blue(0x10), true, {RockBandDrums_BlueCymbal});
    expect_only(Report(Green, CymbalFlag, HatCentre).green(0x10), true, {RockBandDrums_GreenCymbal});
    EXPECT_EQ(rb(Report(Yellow, CymbalFlag, HatUp).yellow(0x10), RockBandDrums_YellowCymbal), 0xEFEF);
}

TEST(PS3Drums, HatDiagonalsCountAsUpAndDown)
{
    for (uint8_t hat : std::initializer_list<uint8_t>{HatUp, HatUpRight, HatUpLeft})
    {
        EXPECT_TRUE(ps3_hat_up(Report(0, 0, hat).buf)) << int(hat);
        EXPECT_FALSE(ps3_hat_down(Report(0, 0, hat).buf)) << int(hat);
    }
    for (uint8_t hat : std::initializer_list<uint8_t>{HatDown, HatDownLeft, 3})
    {
        EXPECT_TRUE(ps3_hat_down(Report(0, 0, hat).buf)) << int(hat);
        EXPECT_FALSE(ps3_hat_up(Report(0, 0, hat).buf)) << int(hat);
    }
    for (uint8_t hat : std::initializer_list<uint8_t>{HatRight, 6, HatCentre, 0x0F})
    {
        EXPECT_FALSE(ps3_hat_up(Report(0, 0, hat).buf)) << int(hat);
        EXPECT_FALSE(ps3_hat_down(Report(0, 0, hat).buf)) << int(hat);
    }
}

TEST(PS3Drums, Rb2PadAndCymbalOfTheSameColour)
{
    // The cymbal velocity goes in red without red being pressed
    Report r = Report(Yellow, PadFlag | CymbalFlag, HatUp).yellow(0x20).red(0x60);
    expect_only(r, true, {RockBandDrums_YellowPad, RockBandDrums_YellowCymbal});
    EXPECT_EQ(rb(r, RockBandDrums_YellowPad), 0xDFDF);
    EXPECT_EQ(rb(r, RockBandDrums_YellowCymbal), 0x9F9F);
}

TEST(PS3Drums, Rb2PadAndCymbalOfDifferentColours)
{
    Report r = Report(Red | Blue, PadFlag | CymbalFlag, HatDown).red(0x20).blue(0x30);
    expect_only(r, true, {RockBandDrums_RedPad, RockBandDrums_BlueCymbal});
}

TEST(PS3Drums, Kicks)
{
    EXPECT_TRUE(ps3_rb_drum_button(Report(Kick1, 0).buf, RockBandDrums_Kick1Pedal));
    EXPECT_FALSE(ps3_rb_drum_button(Report(Kick1, 0).buf, RockBandDrums_Kick2Pedal));
    EXPECT_TRUE(ps3_rb_drum_button(Report(Kick2, 0).buf, RockBandDrums_Kick2Pedal));
    EXPECT_FALSE(ps3_rb_drum_button(Report(0, 0).buf, RockBandDrums_Kick1Pedal));
}

TEST(PS3Drums, GuitarHeroVelocities)
{
    Report r = Report(0, 0).yellow(1).red(2).green(64).blue(100).kick(127).orange(10);
    EXPECT_EQ(ps3_gh_drum_axis(r.buf, GuitarHeroDrums_YellowPad), 1 << 9);
    EXPECT_EQ(ps3_gh_drum_axis(r.buf, GuitarHeroDrums_RedPad), 2 << 9);
    EXPECT_EQ(ps3_gh_drum_axis(r.buf, GuitarHeroDrums_GreenPad), 64 << 9);
    EXPECT_EQ(ps3_gh_drum_axis(r.buf, GuitarHeroDrums_BluePad), 100 << 9);
    EXPECT_EQ(ps3_gh_drum_axis(r.buf, GuitarHeroDrums_KickPedal), 127 << 9);
    EXPECT_EQ(ps3_gh_drum_axis(r.buf, GuitarHeroDrums_OrangePad), 10 << 9);
    for (auto axis : {GuitarHeroDrums_RedPad, GuitarHeroDrums_YellowPad, GuitarHeroDrums_BluePad,
                      GuitarHeroDrums_OrangePad, GuitarHeroDrums_GreenPad, GuitarHeroDrums_KickPedal})
        EXPECT_EQ(ps3_gh_drum_axis(Report(0, 0).buf, axis), 0) << axis;
}
