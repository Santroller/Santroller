// Decoding 360 drum kits in xinput_tick_helpers.h. Reports are built byte by byte from
// PlasticBand Instruments/4-Lane Drums/Xbox 360.md and the Guitar Hero kit layout in
// XInputGuitarHeroDrums_Data_t:
//   byte 2: up, down, left, right, start, back, left thumb (kick 2 / GH flag), right thumb (pad flag)
//   byte 3: LB (kick 1), RB (cymbal flag), guide, -, A (green), B (red), X (blue), Y (yellow)
//   RB velocities: red = left stick X, yellow = left stick Y, blue = right stick X,
//   green = right stick Y. Red / blue are positive, yellow / green negative while hit, and
//   harder hits are closer to 0, as RB1 kits always send 0.
#include <gtest/gtest.h>
#include <algorithm>
#include <cstring>
#include "devices/usb/host/xinput_tick_helpers.h"

namespace
{
enum : uint8_t
{
    Up = 0x01,
    Down = 0x02,
    Kick2 = 0x40,
    PadFlag = 0x80,
};
enum : uint8_t
{
    Kick1 = 0x01,
    CymbalFlag = 0x02,
    Green = 0x10,
    Red = 0x20,
    Blue = 0x40,
    Yellow = 0x80,
};

struct Report
{
    uint8_t buf[20] = {0x00, 0x14};
    Report(uint8_t b2, uint8_t b3)
    {
        buf[2] = b2;
        buf[3] = b3;
    }
    Report &velocity(size_t offset, uint16_t v)
    {
        memcpy(&buf[offset], &v, sizeof(v));
        return *this;
    }
    Report &red(uint16_t v) { return velocity(6, v); }
    Report &yellow(uint16_t v) { return velocity(8, v); }
    Report &blue(uint16_t v) { return velocity(10, v); }
    Report &green(uint16_t v) { return velocity(12, v); }
};

proto_Output rb_axis(RockBandDrumsAxisType axis)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_rbDrumAxis_tag;
    o.mapping.rbDrumAxis = axis;
    return o;
}

proto_Output rb_button(RockBandDrumsButtonType button)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_rbDrumButton_tag;
    o.mapping.rbDrumButton = button;
    return o;
}

proto_Output gh_axis(GuitarHeroDrumsAxisType axis)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_ghDrumAxis_tag;
    o.mapping.ghDrumAxis = axis;
    return o;
}

proto_Output gamepad_button(proto_GamepadButtonType b)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_gamepadButton_tag;
    o.mapping.gamepadButton = b;
    return o;
}

uint16_t rb(const Report &r, RockBandDrumsAxisType axis, bool rb2)
{
    auto o = rb_axis(axis);
    return xinput_tick_analog_impl(r.buf, RockBandDrums, o, rb2);
}

const RockBandDrumsAxisType all_rb_axes[] = {
    RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad,
    RockBandDrums_YellowCymbal, RockBandDrums_BlueCymbal, RockBandDrums_GreenCymbal};

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

TEST(XInputDrums, Rb1PadsAreFaceButtonsWithoutVelocity)
{
    Report r(0, Red | Yellow | Blue | Green);
    for (auto axis : {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad})
        EXPECT_EQ(rb(r, axis, false), UINT16_MAX) << axis;
    expect_only(r, false, {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad});
    expect_only(Report(0, 0), false, {});
}

TEST(XInputDrums, Rb2FaceButtonsWithoutAFlagAreNavigation)
{
    Report r(Up, Red | Green);
    expect_only(r, true, {});
    auto a = gamepad_button(Gamepad_A);
    auto up = gamepad_button(Gamepad_DpadUp);
    EXPECT_TRUE(xinput_tick_digital_impl(r.buf, RockBandDrums, a, false, true));
    EXPECT_TRUE(xinput_tick_digital_impl(r.buf, RockBandDrums, up, false, true));
}

TEST(XInputDrums, Rb2HitsAreNotNavigation)
{
    Report r(PadFlag | Up, Yellow);
    for (auto b : {Gamepad_Y, Gamepad_DpadUp})
    {
        auto o = gamepad_button(b);
        EXPECT_FALSE(xinput_tick_digital_impl(r.buf, RockBandDrums, o, false, true)) << b;
    }
    auto start = gamepad_button(Gamepad_Start);
    Report s(PadFlag | 0x10, Yellow);
    EXPECT_TRUE(xinput_tick_digital_impl(s.buf, RockBandDrums, start, false, true));
}

TEST(XInputDrums, Rb2PadVelocities)
{
    Report hard(PadFlag, Red | Yellow | Blue | Green);
    hard.red(0x0000).yellow(0xFFFF).blue(0x0000).green(0xFFFF);
    for (auto axis : {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad})
        EXPECT_EQ(rb(hard, axis, true), UINT16_MAX) << axis;
    expect_only(hard, true, {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad});

    // RB1 kits send 0 for every velocity, which has to read as a hard hit
    Report zero(PadFlag, Red | Yellow | Blue | Green);
    for (auto axis : {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad})
        EXPECT_EQ(rb(zero, axis, true), UINT16_MAX) << axis;

    Report soft(PadFlag, Red | Yellow | Blue | Green);
    soft.red(0x7FFF).yellow(0x8000).blue(0x7FFF).green(0x8000);
    for (auto axis : {RockBandDrums_RedPad, RockBandDrums_YellowPad, RockBandDrums_BluePad, RockBandDrums_GreenPad})
        EXPECT_EQ(rb(soft, axis, true), 1) << axis;

    Report mid(PadFlag, Red | Yellow);
    mid.red(0x4000).yellow(0xBFFF);
    EXPECT_EQ(rb(mid, RockBandDrums_RedPad, true), 0x7FFF);
    EXPECT_EQ(rb(mid, RockBandDrums_YellowPad, true), 0x7FFF);
}

TEST(XInputDrums, Rb2CymbalColourComesFromTheDpad)
{
    expect_only(Report(Up, CymbalFlag | Yellow).yellow(0xFFFF), true, {RockBandDrums_YellowCymbal});
    expect_only(Report(Down, CymbalFlag | Blue).blue(0), true, {RockBandDrums_BlueCymbal});
    expect_only(Report(0, CymbalFlag | Green).green(0xFFFF), true, {RockBandDrums_GreenCymbal});
}

TEST(XInputDrums, Rb2PadAndCymbalOfDifferentColours)
{
    // Red pad + yellow cymbal: yellow is the cymbal (d-pad up), red stays a pad
    Report r(PadFlag | Up, CymbalFlag | Red | Yellow);
    r.red(0).yellow(0xFFFF);
    expect_only(r, true, {RockBandDrums_RedPad, RockBandDrums_YellowCymbal});

    // Yellow pad + green cymbal: no d-pad, so green is the cymbal
    Report g(PadFlag, CymbalFlag | Yellow | Green);
    g.yellow(0xFFFF).green(0xFFFF);
    expect_only(g, true, {RockBandDrums_YellowPad, RockBandDrums_GreenCymbal});
}

TEST(XInputDrums, Rb2PadAndCymbalOfTheSameColour)
{
    // The cymbal's velocity goes in redVelocity, without red being pressed
    Report r(PadFlag | Up, CymbalFlag | Yellow);
    r.red(0x4000).yellow(0xFFFF);
    expect_only(r, true, {RockBandDrums_YellowPad, RockBandDrums_YellowCymbal});
    EXPECT_EQ(rb(r, RockBandDrums_YellowCymbal, true), 0x7FFF);
    EXPECT_EQ(rb(r, RockBandDrums_YellowPad, true), UINT16_MAX);

    Report b(PadFlag | Down, CymbalFlag | Blue);
    b.red(0x4000).blue(0);
    expect_only(b, true, {RockBandDrums_BluePad, RockBandDrums_BlueCymbal});

    Report g(PadFlag, CymbalFlag | Green);
    g.red(0x4000).green(0xFFFF);
    expect_only(g, true, {RockBandDrums_GreenPad, RockBandDrums_GreenCymbal});
}

TEST(XInputDrums, Kicks)
{
    Report r(Kick2, Kick1);
    auto k1 = rb_button(RockBandDrums_Kick1Pedal);
    auto k2 = rb_button(RockBandDrums_Kick2Pedal);
    EXPECT_TRUE(xinput_tick_digital_impl(r.buf, RockBandDrums, k1, false, true));
    EXPECT_TRUE(xinput_tick_digital_impl(r.buf, RockBandDrums, k2, false, true));
    Report none(0, 0);
    EXPECT_FALSE(xinput_tick_digital_impl(none.buf, RockBandDrums, k1, false, false));
    EXPECT_FALSE(xinput_tick_digital_impl(none.buf, RockBandDrums, k2, false, false));
}

TEST(XInputDrums, PressureIsTheVelocity)
{
    Report r(PadFlag, Red);
    r.red(0x4000);
    auto o = rb_axis(RockBandDrums_RedPad);
    EXPECT_EQ(xinput_tick_button_pressure_impl(r.buf, RockBandDrums, o, false, true), 0x7FFF);
    auto y = rb_axis(RockBandDrums_YellowPad);
    EXPECT_EQ(xinput_tick_button_pressure_impl(r.buf, RockBandDrums, y, false, true), 0);
}

TEST(XInputDrums, GuitarHeroVelocitiesAre7BitMidi)
{
    // green, red, yellow, blue, orange, kick velocities at bytes 8 - 13
    uint8_t buf[20] = {0x00, 0x14};
    buf[8] = 0x7F;
    buf[9] = 0x40;
    buf[13] = 0x01;
    auto green = gh_axis(GuitarHeroDrums_GreenPad);
    auto red = gh_axis(GuitarHeroDrums_RedPad);
    auto yellow = gh_axis(GuitarHeroDrums_YellowPad);
    auto kick = gh_axis(GuitarHeroDrums_KickPedal);
    EXPECT_EQ(xinput_tick_analog_impl(buf, GuitarHeroDrums, green), 0x7F << 9);
    EXPECT_EQ(xinput_tick_analog_impl(buf, GuitarHeroDrums, red), 0x40 << 9);
    EXPECT_EQ(xinput_tick_analog_impl(buf, GuitarHeroDrums, yellow), 0);
    EXPECT_EQ(xinput_tick_analog_impl(buf, GuitarHeroDrums, kick), 1 << 9);
    EXPECT_TRUE(xinput_tick_digital_impl(buf, GuitarHeroDrums, green));
    EXPECT_FALSE(xinput_tick_digital_impl(buf, GuitarHeroDrums, yellow));
}
