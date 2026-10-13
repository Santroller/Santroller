// Decoding Santroller 1 reports in src/devices/santroller_v1_decode.cpp. Reports are built
// byte by byte from the old firmware's include/reports/pc_reports.h:
//   byte 0: report id (1), bytes 1-2: buttons (layout depends on the subtype),
//   byte 3: dpad as a hat (0..7, 8 = neutral) for every subtype, dance pads included,
//   byte 4 on: axes (layout depends on the subtype)
#include <gtest/gtest.h>
#include "protocols/santroller_v1.hpp"

namespace
{
struct Report
{
    uint8_t buf[32] = {0x01, 0x00, 0x00, 0x08};
    Report &set(uint8_t index, uint8_t value)
    {
        buf[index] = value;
        return *this;
    }
};

proto_Output gamepad_button(proto_GamepadButtonType b)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_gamepadButton_tag;
    o.mapping.gamepadButton = b;
    return o;
}

proto_Output gamepad_axis(proto_GamepadAxisType a)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_gamepadAxis_tag;
    o.mapping.gamepadAxis = a;
    return o;
}

bool digital(const Report &r, SubType subtype, proto_Output o)
{
    return santroller_v1_tick_digital(r.buf, subtype, o);
}

uint16_t analog(const Report &r, SubType subtype, proto_Output o)
{
    return santroller_v1_tick_analog(r.buf, subtype, o);
}
} // namespace

TEST(SantrollerV1, SubtypeUsesTheOldNumbering)
{
    EXPECT_EQ(santroller_v1_subtype(0x0100), Gamepad);
    EXPECT_EQ(santroller_v1_subtype(0x0200), Dancepad);
    EXPECT_EQ(santroller_v1_subtype(0x0300), GuitarHeroGuitar);
    EXPECT_EQ(santroller_v1_subtype(0x0800), DjHeroTurntable);
    EXPECT_EQ(santroller_v1_subtype(0x0900), StageKit);
    EXPECT_EQ(santroller_v1_subtype(0x1000), ProKeys);
    EXPECT_EQ(santroller_v1_subtype(0x1100), ProGuitarMustang);
    EXPECT_EQ(santroller_v1_subtype(0x1200), ProGuitarSquier);
    // these moved when the subtypes were renumbered
    EXPECT_EQ(santroller_v1_subtype(0x1500), DisneyInfinity);
    EXPECT_EQ(santroller_v1_subtype(0x1600), Skylanders);
    EXPECT_EQ(santroller_v1_subtype(0x1700), LegoDimensions);
    EXPECT_EQ(santroller_v1_subtype(0x1800), Taiko);
    // keyboard / mouse reports 0, and anything unknown sends the gamepad report
    EXPECT_EQ(santroller_v1_subtype(0x0000), Gamepad);
    EXPECT_EQ(santroller_v1_subtype(0x1300), Gamepad);
    EXPECT_EQ(santroller_v1_subtype(0x1900), Gamepad);
    // only the high byte matters
    EXPECT_EQ(santroller_v1_subtype(0x09FF), StageKit);
}

TEST(SantrollerV1, DancepadDpadIsAHat)
{
    // 1 is up + right as a hat, it would be just up as a bitmask
    auto r = Report().set(3, 1);
    EXPECT_TRUE(digital(r, Dancepad, gamepad_button(Gamepad_DpadUp)));
    EXPECT_TRUE(digital(r, Dancepad, gamepad_button(Gamepad_DpadRight)));
    EXPECT_FALSE(digital(r, Dancepad, gamepad_button(Gamepad_DpadDown)));

    auto neutral = Report();
    EXPECT_FALSE(digital(neutral, Dancepad, gamepad_button(Gamepad_DpadUp)));
    EXPECT_FALSE(digital(neutral, Dancepad, gamepad_button(Gamepad_DpadDown)));
    EXPECT_FALSE(digital(neutral, Dancepad, gamepad_button(Gamepad_DpadLeft)));
    EXPECT_FALSE(digital(neutral, Dancepad, gamepad_button(Gamepad_DpadRight)));
}

TEST(SantrollerV1, GamepadButtons)
{
    // byte 1: x, a, b, y, lb, rb, l2, r2  byte 2: back, start, l3, r3, guide, capture
    EXPECT_TRUE(digital(Report().set(1, 0x01), Gamepad, gamepad_button(Gamepad_X)));
    EXPECT_TRUE(digital(Report().set(1, 0x02), Gamepad, gamepad_button(Gamepad_A)));
    EXPECT_TRUE(digital(Report().set(1, 0x04), Gamepad, gamepad_button(Gamepad_B)));
    EXPECT_TRUE(digital(Report().set(1, 0x08), Gamepad, gamepad_button(Gamepad_Y)));
    EXPECT_TRUE(digital(Report().set(2, 0x10), Gamepad, gamepad_button(Gamepad_Guide)));
    EXPECT_TRUE(digital(Report().set(2, 0x20), Gamepad, gamepad_button(Gamepad_Capture)));
    EXPECT_FALSE(digital(Report().set(1, 0x01), Gamepad, gamepad_button(Gamepad_A)));
}

TEST(SantrollerV1, GamepadReportSubtypesHaveSticks)
{
    // byte 4: left x, 5: left y (down at 0xFF), 6: right x, 7: right y, 8: lt, 9: rt
    auto r = Report().set(4, 0xFF).set(5, 0x00).set(8, 0x80);
    for (SubType subtype : {Gamepad, Dancepad, Taiko, Skylanders, DisneyInfinity, LegoDimensions, StageKit})
    {
        EXPECT_EQ(analog(r, subtype, gamepad_axis(Gamepad_LeftStickX)), 0xFFFF) << subtype;
        // HID y is down at the max, so up reads as the max here
        EXPECT_EQ(analog(r, subtype, gamepad_axis(Gamepad_LeftStickY)), 0xFFFF) << subtype;
        EXPECT_EQ(analog(r, subtype, gamepad_axis(Gamepad_LeftTrigger)), 0x8080) << subtype;
    }
}

TEST(SantrollerV1, StageKitFaceButtonOrder)
{
    // byte 1: a, b, x, y, unlike the gamepad's x, a, b, y
    EXPECT_TRUE(digital(Report().set(1, 0x01), StageKit, gamepad_button(Gamepad_A)));
    EXPECT_TRUE(digital(Report().set(1, 0x02), StageKit, gamepad_button(Gamepad_B)));
    EXPECT_TRUE(digital(Report().set(1, 0x04), StageKit, gamepad_button(Gamepad_X)));
    EXPECT_TRUE(digital(Report().set(1, 0x08), StageKit, gamepad_button(Gamepad_Y)));
    EXPECT_FALSE(digital(Report().set(1, 0x01), StageKit, gamepad_button(Gamepad_X)));
}

TEST(SantrollerV1, GuitarHeroGuitarTapFrets)
{
    // byte 4: whammy, 5: slider (GH5 values), 6: tilt
    proto_Output tap = {};
    tap.which_mapping = proto_Output_ghButton_tag;
    tap.mapping.ghButton = GuitarHeroGuitar_TapRed;
    EXPECT_TRUE(digital(Report().set(5, 0x4D), GuitarHeroGuitar, tap));
    EXPECT_TRUE(digital(Report().set(5, 0x30), GuitarHeroGuitar, tap));
    EXPECT_FALSE(digital(Report().set(5, 0x15), GuitarHeroGuitar, tap));
    EXPECT_FALSE(digital(Report().set(5, 0x80), GuitarHeroGuitar, tap));
}

TEST(SantrollerV1, LiveGuitarGhtv)
{
    // byte 2: ghtv (l3), guide
    proto_Output ghtv = {};
    ghtv.which_mapping = proto_Output_ghlButton_tag;
    ghtv.mapping.ghlButton = GuitarHeroLiveGuitar_GHTV;
    EXPECT_TRUE(digital(Report().set(2, 0x01), LiveGuitar, ghtv));
    EXPECT_FALSE(digital(Report().set(2, 0x02), LiveGuitar, ghtv));
}

TEST(SantrollerV1, GuitarHeroDrumVelocities)
{
    // byte 4 on: green, red, yellow, blue, orange, kick velocities
    proto_Output kick = {};
    kick.which_mapping = proto_Output_ghDrumAxis_tag;
    kick.mapping.ghDrumAxis = GuitarHeroDrums_KickPedal;
    proto_Output red = kick;
    red.mapping.ghDrumAxis = GuitarHeroDrums_RedPad;

    auto r = Report().set(5, 0x40).set(9, 0xFF);
    EXPECT_EQ(analog(r, GuitarHeroDrums, red), 0x4040);
    EXPECT_EQ(analog(r, GuitarHeroDrums, kick), 0xFFFF);
    EXPECT_TRUE(digital(r, GuitarHeroDrums, red));
    EXPECT_FALSE(digital(Report(), GuitarHeroDrums, red));
}

TEST(SantrollerV1, RockBandDrums)
{
    // byte 1: a, b, x, y, pad flag, cymbal flag, kick 1, kick 2
    // byte 4 on: green, red, yellow, blue pad, then green, yellow, blue cymbal velocities
    proto_Output yellow_cymbal = {};
    yellow_cymbal.which_mapping = proto_Output_rbDrumAxis_tag;
    yellow_cymbal.mapping.rbDrumAxis = RockBandDrums_YellowCymbal;
    proto_Output kick2 = {};
    kick2.which_mapping = proto_Output_rbDrumButton_tag;
    kick2.mapping.rbDrumButton = RockBandDrums_Kick2Pedal;

    auto hit = Report().set(1, 0x01 | 0x20).set(3, 0).set(9, 0x7F);
    EXPECT_EQ(analog(hit, RockBandDrums, yellow_cymbal), 0x7F7F);
    EXPECT_TRUE(digital(hit, RockBandDrums, yellow_cymbal));
    // while a hit is flagged, the face buttons and dpad aren't navigation
    EXPECT_FALSE(digital(hit, RockBandDrums, gamepad_button(Gamepad_A)));
    EXPECT_FALSE(digital(hit, RockBandDrums, gamepad_button(Gamepad_DpadUp)));

    auto nav = Report().set(1, 0x01).set(3, 0);
    EXPECT_TRUE(digital(nav, RockBandDrums, gamepad_button(Gamepad_A)));
    EXPECT_TRUE(digital(nav, RockBandDrums, gamepad_button(Gamepad_DpadUp)));

    EXPECT_TRUE(digital(Report().set(1, 0x80), RockBandDrums, kick2));
    EXPECT_FALSE(digital(Report().set(1, 0x40), RockBandDrums, kick2));
}

TEST(SantrollerV1, ProGuitar)
{
    // byte 1: x, a, b, y, back, start, guide, green  byte 2: red, yellow, blue, orange, pedal
    // byte 4: tilt, 5-10: frets low E to high E, 11-16: velocities
    proto_Output green = {};
    green.which_mapping = proto_Output_proButton_tag;
    green.mapping.proButton = ProGuitar_Green;
    proto_Output pedal = green;
    pedal.mapping.proButton = ProGuitar_Pedal;
    proto_Output low_e = {};
    low_e.which_mapping = proto_Output_proAxis_tag;
    low_e.mapping.proAxis = ProGuitar_LowEFret;
    proto_Output high_e_velocity = low_e;
    high_e_velocity.mapping.proAxis = ProGuitar_HighEFretVelocity;

    EXPECT_TRUE(digital(Report().set(1, 0x80), ProGuitarMustang, green));
    EXPECT_TRUE(digital(Report().set(2, 0x10), ProGuitarSquier, pedal));
    EXPECT_TRUE(digital(Report().set(1, 0x02), ProGuitarMustang, gamepad_button(Gamepad_A)));
    EXPECT_FALSE(digital(Report().set(1, 0x02), ProGuitarMustang, gamepad_button(Gamepad_X)));
    EXPECT_EQ(analog(Report().set(5, 3), ProGuitarMustang, low_e), 3 << 11);
    EXPECT_EQ(analog(Report().set(16, 0x40), ProGuitarSquier, high_e_velocity), 0x40 << 9);
}

TEST(SantrollerV1, ProKeys)
{
    // byte 1: x, a, b, y, back, start, guide, overdrive  byte 2: pedal
    // bytes 4-6: keys, msb first, 7: pedal, 8: touch pad, 9-13: velocities (bit 7 of 9 is key 25)
    proto_Output key1 = {};
    key1.which_mapping = proto_Output_proKeySingle_tag;
    key1.mapping.proKeySingle = 1;
    proto_Output key25 = key1;
    key25.mapping.proKeySingle = 25;
    proto_Output overdrive = {};
    overdrive.which_mapping = proto_Output_proKeyboardButton_tag;
    overdrive.mapping.proKeyboardButton = ProKeyboardOverdrive;

    auto r = Report().set(4, 0x80).set(9, 0x40);
    EXPECT_TRUE(digital(r, ProKeys, key1));
    EXPECT_FALSE(digital(r, ProKeys, key25));
    EXPECT_EQ(santroller_v1_tick_button_pressure(r.buf, ProKeys, key1), 0x40 << 9);
    EXPECT_TRUE(digital(Report().set(9, 0x80), ProKeys, key25));
    EXPECT_TRUE(digital(Report().set(1, 0x80), ProKeys, overdrive));
    EXPECT_FALSE(digital(Report().set(1, 0x80), ProKeys, gamepad_button(Gamepad_Y)));
}
