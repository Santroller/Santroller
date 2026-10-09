// lib/gip_common/gip_button_mapping.cpp: decoding a controller's input reports once they're in
// gip_device_t::raw_input. Reports are built byte by byte from the documented layouts, not from
// the firmware's structs:
//   gamepad          [MS-GIPUSB] Table 57 (also xpad xpadone_process_packet)
//   Rock Band guitar PlasticBand Instruments/5-Fret Guitar/Rock Band/Xbox One.md
//   Rock Band drums  PlasticBand Instruments/4-Lane Drums/Xbox One.md
//   GHL guitar       PlasticBand Instruments/6-Fret Guitar/Xbox One.md (message 0x21)
// Axes come out as 0 - 65535 like the firmware's other USB hosts (e.g. ps3_host.cpp shifts 8 bit
// axes left by 8, xinput_tick_helpers.h maps signed 16 bit axes with ^ 0x8000).
#include <gtest/gtest.h>

#include "gip_button_mapping.h"
#include "gip_device.h"
#include "gip_fakes.hpp"
#include "gip_test_support.hpp"
#include "plasticband_metadata.hpp"
#include "protocols/rb_pickup.hpp"

using namespace gip_test;
namespace pb = gip_test::plasticband;

namespace
{
proto_Output out(pb_size_t tag)
{
    proto_Output o = {};
    o.which_mapping = tag;
    return o;
}
proto_Output gamepad_button(proto_GamepadButtonType b)
{
    proto_Output o = out(proto_Output_gamepadButton_tag);
    o.mapping.gamepadButton = b;
    return o;
}
proto_Output gamepad_axis(proto_GamepadAxisType a)
{
    proto_Output o = out(proto_Output_gamepadAxis_tag);
    o.mapping.gamepadAxis = a;
    return o;
}
proto_Output rb_button(proto_RockBandGuitarButtonType b)
{
    proto_Output o = out(proto_Output_rbButton_tag);
    o.mapping.rbButton = b;
    return o;
}
proto_Output rb_axis(proto_RockBandGuitarAxisType a)
{
    proto_Output o = out(proto_Output_rbAxis_tag);
    o.mapping.rbAxis = a;
    return o;
}
proto_Output drum_button(proto_RockBandDrumsButtonType b)
{
    proto_Output o = out(proto_Output_rbDrumButton_tag);
    o.mapping.rbDrumButton = b;
    return o;
}
proto_Output drum_axis(proto_RockBandDrumsAxisType a)
{
    proto_Output o = out(proto_Output_rbDrumAxis_tag);
    o.mapping.rbDrumAxis = a;
    return o;
}
proto_Output ghl_button(proto_GuitarHeroLiveGuitarButtonType b)
{
    proto_Output o = out(proto_Output_ghlButton_tag);
    o.mapping.ghlButton = b;
    return o;
}
proto_Output ghl_axis(proto_GuitarHeroLiveGuitarAxisType a)
{
    proto_Output o = out(proto_Output_ghlAxis_tag);
    o.mapping.ghlAxis = a;
    return o;
}

void put16(Bytes &b, size_t at, uint16_t v)
{
    b[at] = (uint8_t)v;
    b[at + 1] = (uint8_t)(v >> 8);
}

class GipInput : public ::testing::Test
{
protected:
    gip_device_t dev;
    Recorder rec;
    uint8_t seq = 0;

    void SetUp() override
    {
        gip_fake::reset();
        gip_device_init(&dev);
        dev.user_context = &rec;
        dev.interface = &Recorder::interface;
        rec.device = &dev;
    }
    void TearDown() override
    {
        gip_device_cleanup(&dev);
    }
    void report(const Bytes &payload, uint8_t type = 0x20)
    {
        if (++seq == 0)
            seq = 1;
        Bytes wire = packet(type, 0x00, seq, payload);
        gip_device_process_incoming(&dev, wire.data(), (uint16_t)wire.size());
    }
    bool digital(proto_Output o)
    {
        return gip_tick_digital(dev.raw_input, dev.subtype, dev.capture, &o);
    }
    uint16_t analog(proto_Output o)
    {
        return gip_tick_analog(dev.raw_input, dev.subtype, &o);
    }
    void load_metadata(const Bytes &md)
    {
        for (auto &f : fragments(GIP_DEVICE_DESCRIPTOR, 0x01, md))
            gip_device_process_incoming(&dev, f.data(), (uint16_t)f.size());
        Bytes done = fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x01, md.size());
        gip_device_process_incoming(&dev, done.data(), (uint16_t)done.size());
    }
};
} // namespace

// --- Gamepad ---

TEST_F(GipInput, GamepadButtons)
{
    load_metadata(pb::MICROSOFT_GAMEPAD_METADATA);
    ASSERT_EQ(dev.subtype, Gamepad);
    struct
    {
        int byte;
        uint8_t bit;
        proto_GamepadButtonType button;
    } table[] = {
        {0, 0x04, Gamepad_Start}, // Menu
        {0, 0x08, Gamepad_Back},  // View
        {0, 0x10, Gamepad_A},
        {0, 0x20, Gamepad_B},
        {0, 0x40, Gamepad_X},
        {0, 0x80, Gamepad_Y},
        {1, 0x01, Gamepad_DpadUp},
        {1, 0x02, Gamepad_DpadDown},
        {1, 0x04, Gamepad_DpadLeft},
        {1, 0x08, Gamepad_DpadRight},
        {1, 0x10, Gamepad_LeftShoulder},
        {1, 0x20, Gamepad_RightShoulder},
        {1, 0x40, Gamepad_LeftThumbClick},
        {1, 0x80, Gamepad_RightThumbClick},
    };
    for (auto &row : table)
    {
        Bytes payload(14, 0);
        payload[row.byte] = row.bit;
        report(payload);
        for (auto &other : table)
            EXPECT_EQ(digital(gamepad_button(other.button)), &other == &row) << "byte " << row.byte << " bit " << (int)row.bit;
    }
}

// Sticks are signed 16 bit, -32768 to 32767 (Table 57)
TEST_F(GipInput, GamepadSticks)
{
    dev.subtype = Gamepad;
    Bytes payload(14, 0);
    put16(payload, 6, (uint16_t)-32768);
    put16(payload, 8, 32767);
    put16(payload, 10, 0);
    put16(payload, 12, (uint16_t)-1);
    report(payload);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_LeftStickX)), 0x0000);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_LeftStickY)), 0xFFFF);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_RightStickX)), 0x8000);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_RightStickY)), 0x7FFF);
}

// Triggers are 10 bit, "0 - 1023" ([MS-GIPUSB] Table 57; PlasticBand Xbox One Base "fully pressed
// at 0x03FF"; xpad registers ABS_Z / ABS_RZ as 0 - 1023 for Xbox One pads)
TEST_F(GipInput, GamepadTriggersAtRestAndFull)
{
    dev.subtype = Gamepad;
    Bytes payload(14, 0);
    report(payload);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_LeftTrigger)), 0);
    put16(payload, 2, 0x3FF);
    put16(payload, 4, 0x3FF);
    report(payload);
    EXPECT_GE(analog(gamepad_axis(Gamepad_LeftTrigger)), 0xFF00);
    EXPECT_GE(analog(gamepad_axis(Gamepad_RightTrigger)), 0xFF00);
}

// The triggers are 10 bit (lib/gip_common/gip_button_mapping.cpp, gip_tick_analog default case), so
// they're scaled up by << 6 (not << 8 like an 8 bit value, which would keep only their low byte):
// the axis rises with the trigger and half pressed (512) reads as half the axis.
TEST_F(GipInput, GamepadTriggersAreMonotonic)
{
    dev.subtype = Gamepad;
    Bytes payload(14, 0);
    uint16_t last = 0;
    int first_drop = -1;
    for (uint16_t raw = 0; raw <= 0x3FF && first_drop < 0; raw++)
    {
        put16(payload, 2, raw);
        report(payload);
        uint16_t value = analog(gamepad_axis(Gamepad_LeftTrigger));
        if (value < last)
            first_drop = raw;
        last = value;
    }
    EXPECT_EQ(first_drop, -1) << "trigger value drops at raw " << first_drop;
    put16(payload, 2, 0x200);
    report(payload);
    EXPECT_NEAR(analog(gamepad_axis(Gamepad_LeftTrigger)), 0x8000, 0x200);
}

// The Share button is console function 0x01 in the IConsoleFunctionMap extension at the end of
// the report ([MS-GIPUSB] 3.1.5.6.1.3.1; non-zero IDs can be in any order). With the extension
// declared in metadata, the 0x20 report is 32 bytes and the map starts at byte 14.
TEST_F(GipInput, ShareFromTheConsoleFunctionMap)
{
    load_metadata(metadata({"Windows.Xbox.Input.Gamepad"}, {GUID_ICONTROLLER, GUID_IGAMEPAD, GUID_ICONSOLE_FUNCTION_MAP, GUID_INAVIGATION},
                           {{0x20, 0x20, true}, {0x09, 9, false}}));
    ASSERT_EQ(dev.subtype, Gamepad);
    Bytes payload(32, 0);
    report(payload);
    EXPECT_FALSE(digital(gamepad_button(Gamepad_Capture)));
    payload[14] = 0x01;
    report(payload);
    EXPECT_TRUE(digital(gamepad_button(Gamepad_Capture)));
    payload[14] = 0x07;
    payload[15] = 0x01;
    report(payload);
    EXPECT_TRUE(digital(gamepad_button(Gamepad_Capture)));
    payload[14] = 0x00;
    payload[15] = 0x00;
    report(payload);
    EXPECT_FALSE(digital(gamepad_button(Gamepad_Capture)));
}

TEST_F(GipInput, NoShareWithoutTheExtension)
{
    load_metadata(pb::MICROSOFT_GAMEPAD_METADATA);
    Bytes payload(32, 0);
    payload[14] = 0x01;
    report(payload);
    EXPECT_FALSE(digital(gamepad_button(Gamepad_Capture)));
}

// --- Rock Band guitar ---

TEST_F(GipInput, RockBandGuitarFrets)
{
    load_metadata(pb::MADCATZ_STRATOCASTER_METADATA);
    ASSERT_EQ(dev.subtype, RockBandGuitar);
    // Byte 5: upper frets, byte 6: solo frets; bit 0 green, 1 red, 2 yellow, 3 blue, 4 orange
    const proto_RockBandGuitarButtonType upper[] = {RockBandGuitar_Green, RockBandGuitar_Red, RockBandGuitar_Yellow,
                                                    RockBandGuitar_Blue, RockBandGuitar_Orange};
    const proto_RockBandGuitarButtonType solo[] = {RockBandGuitar_SoloGreen, RockBandGuitar_SoloRed, RockBandGuitar_SoloYellow,
                                                   RockBandGuitar_SoloBlue, RockBandGuitar_SoloOrange};
    for (int i = 0; i < 5; i++)
    {
        Bytes payload(10, 0);
        payload[5] = 1 << i;
        report(payload);
        for (int j = 0; j < 5; j++)
        {
            EXPECT_EQ(digital(rb_button(upper[j])), i == j) << i << " " << j;
            EXPECT_FALSE(digital(rb_button(solo[j])));
        }
        payload[5] = 0;
        payload[6] = 1 << i;
        report(payload);
        for (int j = 0; j < 5; j++)
        {
            EXPECT_EQ(digital(rb_button(solo[j])), i == j) << i << " " << j;
            EXPECT_FALSE(digital(rb_button(upper[j])));
        }
    }
}

// Bytes 0-1 are the navigation buttons (frets double as A/B/X/Y, strum as d-pad up/down)
TEST_F(GipInput, RockBandGuitarNavigation)
{
    dev.subtype = RockBandGuitar;
    Bytes payload(10, 0);
    payload[0] = 0x04 | 0x10; // Menu, green flag
    payload[1] = 0x02;        // strum down
    report(payload);
    EXPECT_TRUE(digital(gamepad_button(Gamepad_Start)));
    EXPECT_TRUE(digital(gamepad_button(Gamepad_A)));
    EXPECT_TRUE(digital(gamepad_button(Gamepad_DpadDown)));
    EXPECT_FALSE(digital(gamepad_button(Gamepad_DpadUp)));
    EXPECT_FALSE(digital(gamepad_button(Gamepad_Back)));
}

// Riffmaster joystick: bytes 10-13, signed, left / down negative
TEST_F(GipInput, RiffmasterJoystick)
{
    load_metadata(pb::PDP_RIFFMASTER_METADATA);
    ASSERT_EQ(dev.subtype, RockBandGuitar);
    Bytes payload(32, 0);
    put16(payload, 10, (uint16_t)-32768);
    put16(payload, 12, 32767);
    report(payload);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_LeftStickX)), 0x0000);
    EXPECT_EQ(analog(gamepad_axis(Gamepad_LeftStickY)), 0xFFFF);
}

// Whammy: byte 3, "0x00 when not pressed to 0xFF when fully pressed"; tilt: byte 2, "0x00 when
// parallel, 0xFF when straight up". As 0 - 65535 axes, like ps3_host / ps4_host (value << 8;
// lib/gip_common/gip_button_mapping.cpp, gip_tick_analog SubType_RockBandGuitar).
TEST_F(GipInput, RockBandGuitarWhammyAndTiltSpanTheAxis)
{
    dev.subtype = RockBandGuitar;
    Bytes payload(10, 0);
    report(payload);
    EXPECT_LE(analog(rb_axis(RockBandGuitar_Whammy)), 0x00FF);
    EXPECT_LE(analog(rb_axis(RockBandGuitar_Tilt)), 0x00FF);
    payload[2] = 0xFF;
    payload[3] = 0xFF;
    report(payload);
    EXPECT_GE(analog(rb_axis(RockBandGuitar_Whammy)), 0xFF00);
    EXPECT_GE(analog(rb_axis(RockBandGuitar_Tilt)), 0xFF00);
}

// Pickup: byte 4, one of 0x00, 0x10, 0x20, 0x30, 0x40 for the five notches (PlasticBand). The
// firmware's other hosts turn a notch into the same axis value whatever the console
// (protocols/rb_pickup.hpp rb_pickup_notch_value, used by ps4_host.cpp), so gip_tick_analog passes
// it the notch from the top nibble.
TEST_F(GipInput, RockBandGuitarPickupNotches)
{
    dev.subtype = RockBandGuitar;
    for (uint8_t notch = 0; notch < 5; notch++)
    {
        Bytes payload(10, 0);
        payload[4] = (uint8_t)(notch << 4);
        report(payload);
        EXPECT_EQ(analog(rb_axis(RockBandGuitar_Pickup)), rb_pickup_notch_value(notch)) << (int)notch;
    }
}

// --- Rock Band drums ---

TEST_F(GipInput, DrumKicks)
{
    load_metadata(pb::PDP_DRUMS_METADATA);
    ASSERT_EQ(dev.subtype, RockBandDrums);
    Bytes payload(6, 0);
    payload[1] = 0x10; // 1st kick
    report(payload);
    EXPECT_TRUE(digital(drum_button(RockBandDrums_Kick1Pedal)));
    EXPECT_FALSE(digital(drum_button(RockBandDrums_Kick2Pedal)));
    payload[1] = 0x20; // 2nd kick
    report(payload);
    EXPECT_FALSE(digital(drum_button(RockBandDrums_Kick1Pedal)));
    EXPECT_TRUE(digital(drum_button(RockBandDrums_Kick2Pedal)));
}

// Velocity nibbles: byte 2 low yellow pad / high red pad, byte 3 low green pad / high blue pad,
// byte 4 low blue cymbal / high yellow cymbal, byte 5 high green cymbal
TEST_F(GipInput, DrumVelocityNibbles)
{
    load_metadata(pb::MADCATZ_DRUMS_METADATA);
    ASSERT_EQ(dev.subtype, RockBandDrums);
    struct
    {
        int byte;
        uint8_t value;
        proto_RockBandDrumsAxisType pad;
    } table[] = {
        {2, 0x07, RockBandDrums_YellowPad},   {2, 0x70, RockBandDrums_RedPad},
        {3, 0x07, RockBandDrums_GreenPad},    {3, 0x70, RockBandDrums_BluePad},
        {4, 0x07, RockBandDrums_BlueCymbal},  {4, 0x70, RockBandDrums_YellowCymbal},
        {5, 0x70, RockBandDrums_GreenCymbal},
    };
    for (auto &row : table)
    {
        Bytes payload(6, 0);
        payload[row.byte] = row.value;
        report(payload);
        for (auto &other : table)
        {
            if (&other == &row)
            {
                EXPECT_GT(analog(drum_axis(other.pad)), 0) << row.byte << " " << (int)row.value;
            }
            else
            {
                EXPECT_EQ(analog(drum_axis(other.pad)), 0) << row.byte << " " << (int)row.value;
            }
        }
    }
    // Harder hits read higher
    Bytes soft(6, 0), hard(6, 0);
    soft[2] = 0x01;
    hard[2] = 0x07;
    report(soft);
    uint16_t s = analog(drum_axis(RockBandDrums_YellowPad));
    report(hard);
    EXPECT_GT(analog(drum_axis(RockBandDrums_YellowPad)), s);
}

// --- Guitar Hero Live guitar (PS3-style message 0x21) ---

namespace
{
Bytes ghl_idle()
{
    Bytes r(27, 0);
    r[2] = 0x0F; // d-pad: 15 = centred
    r[4] = 0x80; // strum bar: 0x80 when not strumming
    r[5] = 0x80; // tilt extremes
    r[6] = 0x80; // whammy: 0x80 when not pressed
    return r;
}
} // namespace

TEST_F(GipInput, GhlFretsAndButtons)
{
    load_metadata(pb::GHL_DONGLE_METADATA);
    ASSERT_EQ(dev.subtype, LiveGuitar);
    struct
    {
        int byte;
        uint8_t bit;
        proto_GuitarHeroLiveGuitarButtonType button;
    } table[] = {
        {0, 0x01, GuitarHeroLiveGuitar_White1}, {0, 0x02, GuitarHeroLiveGuitar_Black1},
        {0, 0x04, GuitarHeroLiveGuitar_Black2}, {0, 0x08, GuitarHeroLiveGuitar_Black3},
        {0, 0x10, GuitarHeroLiveGuitar_White2}, {0, 0x20, GuitarHeroLiveGuitar_White3},
        {1, 0x04, GuitarHeroLiveGuitar_GHTV},
    };
    for (auto &row : table)
    {
        Bytes r = ghl_idle();
        r[row.byte] |= row.bit;
        report(r, 0x21);
        for (auto &other : table)
            EXPECT_EQ(digital(ghl_button(other.button)), &other == &row) << row.byte << " " << (int)row.bit;
    }
    Bytes r = ghl_idle();
    r[1] = 0x01 | 0x02; // Hero Power, Pause
    report(r, 0x21);
    EXPECT_TRUE(digital(gamepad_button(Gamepad_Back)));
    EXPECT_TRUE(digital(gamepad_button(Gamepad_Start)));
}

// Strum bar: 0x00 strumming up, 0xFF strumming down, 0x80 at rest
TEST_F(GipInput, GhlStrum)
{
    dev.subtype = LiveGuitar;
    Bytes r = ghl_idle();
    report(r, 0x21);
    EXPECT_FALSE(digital(ghl_button(GuitarHeroLiveGuitar_StrumUp)));
    EXPECT_FALSE(digital(ghl_button(GuitarHeroLiveGuitar_StrumDown)));
    r[4] = 0x00;
    report(r, 0x21);
    EXPECT_TRUE(digital(ghl_button(GuitarHeroLiveGuitar_StrumUp)));
    r[4] = 0xFF;
    report(r, 0x21);
    EXPECT_TRUE(digital(ghl_button(GuitarHeroLiveGuitar_StrumDown)));
    EXPECT_FALSE(digital(ghl_button(GuitarHeroLiveGuitar_StrumUp)));
}

// D-pad states: 0 up, 1 up-right, 2 right, 3 down-right, 4 down, 5 down-left, 6 left,
// 7 up-left, 15 centred
TEST_F(GipInput, GhlDpad)
{
    dev.subtype = LiveGuitar;
    struct
    {
        uint8_t state;
        bool up, down, left, right;
    } table[] = {
        {0, 1, 0, 0, 0}, {1, 1, 0, 0, 1}, {2, 0, 0, 0, 1}, {3, 0, 1, 0, 1}, {4, 0, 1, 0, 0},
        {5, 0, 1, 1, 0}, {6, 0, 0, 1, 0}, {7, 1, 0, 1, 0}, {15, 0, 0, 0, 0},
    };
    for (auto &row : table)
    {
        Bytes r = ghl_idle();
        r[2] = row.state;
        report(r, 0x21);
        EXPECT_EQ(digital(gamepad_button(Gamepad_DpadUp)), row.up) << (int)row.state;
        EXPECT_EQ(digital(gamepad_button(Gamepad_DpadDown)), row.down) << (int)row.state;
        EXPECT_EQ(digital(gamepad_button(Gamepad_DpadLeft)), row.left) << (int)row.state;
        EXPECT_EQ(digital(gamepad_button(Gamepad_DpadRight)), row.right) << (int)row.state;
    }
}

// The navigation-only 0x20 report a GHL guitar also sends must not replace its 0x21 state
TEST_F(GipInput, GhlIgnoresTheNavigationReport)
{
    dev.subtype = LiveGuitar;
    Bytes r = ghl_idle();
    r[0] = 0x02; // Black 1
    report(r, 0x21);
    report(Bytes(14, 0), 0x20);
    EXPECT_TRUE(digital(ghl_button(GuitarHeroLiveGuitar_Black1)));
}

TEST_F(GipInput, GhlWhammy)
{
    dev.subtype = LiveGuitar;
    Bytes r = ghl_idle();
    r[6] = 0xFF; // fully pressed
    report(r, 0x21);
    EXPECT_GE(analog(ghl_axis(GuitarHeroLiveGuitar_Whammy)), 0xFF00);
}

// Tilt: bytes 19-20, "Maxes out at 0xFF, bottoms out at 0x00" (PlasticBand), so full tilt should
// read near the top of the axis like the PS3 host's tilt << 8 (lib/gip_common/gip_button_mapping.cpp,
// gip_tick_analog SubType_LiveGuitar).
TEST_F(GipInput, GhlTiltSpansTheAxis)
{
    dev.subtype = LiveGuitar;
    Bytes r = ghl_idle();
    put16(r, 19, 0x00FF);
    report(r, 0x21);
    EXPECT_GE(analog(ghl_axis(GuitarHeroLiveGuitar_Tilt)), 0xFF00);
}
