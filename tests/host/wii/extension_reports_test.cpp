// The reports the Santroller hands a Wii Remote when it pretends to be an extension: the real
// mappings writing into the report that src/emulation/wii_extension_input.cpp sets up and finishes
// (buttons inverted to active low), the same steps WiiExtensionEmulationDeviceInstance::process
// runs. Every expected byte is built from wiibrew's bit tables:
//   Classic Controller formats 1, 2, 3 (neutral sticks at the middle of their range, triggers 0)
//   Guitar Hero guitar, GHWT drums (all MIDI data inverted, FF FF in bytes 2 and 3 with no hit),
//   DJ Hero turntable (6 bit signed turntables, not inverted) and TaTaCon (A0 20 50 10 FF, then
//   1 CL RL CR RR 1 1 1). Fixed "1" bits in the button bytes stay set.
#include <gtest/gtest.h>
#include <string.h>
#include <vector>
#include "mappings/mapping_test_support.hpp"
#include "emulation/wii_extension_input.hpp"
#include "protocols/wii.hpp"

namespace
{
proto_Mapping button(GamepadButtonType type) { return gamepad_button(type); }

proto_Mapping gh_button(GuitarHeroGuitarButtonType type)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_ghButton_tag;
    config.mapping.mapping.ghButton = type;
    return config;
}
proto_Mapping gh_axis(GuitarHeroGuitarAxisType type)
{
    proto_Mapping config = trigger_config();
    config.mapping.which_mapping = proto_Output_ghAxis_tag;
    config.mapping.mapping.ghAxis = type;
    return config;
}
proto_Mapping gh_drum(GuitarHeroDrumsAxisType type)
{
    proto_Mapping config = trigger_config();
    config.mapping.which_mapping = proto_Output_ghDrumAxis_tag;
    config.mapping.mapping.ghDrumAxis = type;
    return config;
}
proto_Mapping djh_button(DJHTurntableButtonType type)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_djhButton_tag;
    config.mapping.mapping.djhButton = type;
    return config;
}
proto_Mapping djh_axis(DJHTurntableAxisType type)
{
    proto_Mapping config = stick_config();
    config.mapping.which_mapping = proto_Output_djhAxis_tag;
    config.mapping.mapping.djhAxis = type;
    return config;
}

class WiiReport : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile(Gamepad);
    std::vector<std::unique_ptr<Mapping>> mappings;
    std::vector<FakeInput *> inputs;

    template <typename T>
    void pressed(const proto_Mapping &config)
    {
        auto driven = drive<T>(config, profile);
        driven.set_digital(true);
        mappings.push_back(std::move(driven.mapping));
    }
    template <typename T>
    void at(const proto_Mapping &config, uint16_t value)
    {
        auto driven = drive<T>(config, profile);
        driven.set_analog(value);
        mappings.push_back(std::move(driven.mapping));
    }

    // What the device instance does for one report. format_register is what the Wii Remote left
    // in 0xFE (1 unless it asked for another Classic Controller format)
    std::vector<uint8_t> report(SubType subtype, uint8_t format_register = 1)
    {
        profile->subtype = subtype;
        uint8_t buf[32];
        memset(buf, 0xA5, sizeof(buf));
        uint8_t size = 0, low = 0, high = 0;
        uint8_t format = wii_extension_format_for_subtype(subtype, format_register);
        initialize_wii_extension_report(subtype, format, buf, &size, &low, &high);
        for (auto &mapping : mappings)
        {
            mapping->update_wii(format_register, buf);
        }
        finalize_wii_extension_report(subtype, buf, low, high);
        return std::vector<uint8_t>(buf, buf + size);
    }
};

std::vector<uint8_t> with_cleared(std::vector<uint8_t> report, size_t byte, uint8_t bit)
{
    report[byte] &= ~(1 << bit);
    return report;
}
} // namespace

// Formats and sizes

TEST(WiiReportFormat, OnlyTheClassicControllerFollowsTheRequestedFormat)
{
    for (uint8_t requested : {1, 2, 3})
    {
        EXPECT_EQ(wii_extension_format_for_subtype(Gamepad, requested), requested);
        EXPECT_EQ(wii_extension_format_for_subtype(GuitarHeroGuitar, requested), 3);
        EXPECT_EQ(wii_extension_format_for_subtype(GuitarHeroDrums, requested), 3);
        EXPECT_EQ(wii_extension_format_for_subtype(DjHeroTurntable, requested), 3);
        EXPECT_EQ(wii_extension_format_for_subtype(Taiko, requested), 3);
    }
}

TEST(WiiReportFormat, ReportSizes)
{
    EXPECT_EQ(wii_extension_report_size(Gamepad, 1), 6);
    EXPECT_EQ(wii_extension_report_size(Gamepad, 2), 9);
    EXPECT_EQ(wii_extension_report_size(Gamepad, 3), 8);
    EXPECT_EQ(wii_extension_report_size(GuitarHeroGuitar, 3), 6);
    EXPECT_EQ(wii_extension_report_size(GuitarHeroDrums, 3), 6);
    EXPECT_EQ(wii_extension_report_size(DjHeroTurntable, 3), 6);
    EXPECT_EQ(wii_extension_report_size(Taiko, 3), 6);
}

// Classic Controller

TEST_F(WiiReport, ClassicFormat1Neutral)
{
    // LX = LY = 32, RX = RY = 16, LT = RT = 0, nothing pressed
    EXPECT_EQ(report(Gamepad, 1), (std::vector<uint8_t>{0xA0, 0x20, 0x10, 0x00, 0xFF, 0xFF}));
}

TEST_F(WiiReport, ClassicFormat2Neutral)
{
    // 10 bit sticks at 512: bits 9-2 are 0x80, bits 1-0 zero
    EXPECT_EQ(report(Gamepad, 2), (std::vector<uint8_t>{0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF}));
}

TEST_F(WiiReport, ClassicFormat3Neutral)
{
    EXPECT_EQ(report(Gamepad, 3), (std::vector<uint8_t>{0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0xFF, 0xFF}));
}

TEST_F(WiiReport, ClassicButtonsAreActiveLowInEveryFormat)
{
    // byte, bit within the two button bytes: 4: BDR BDD BLT B- BH B+ BRT 1, 5: BZL BB BY BA BX BZR BDL BDU
    const struct
    {
        GamepadButtonType button;
        uint8_t byte;
        uint8_t bit;
    } buttons[] = {
        {Gamepad_DpadRight, 0, 7}, {Gamepad_DpadDown, 0, 6},     {Gamepad_Back, 0, 4},
        {Gamepad_Guide, 0, 3},     {Gamepad_Start, 0, 2},        {Gamepad_LeftShoulder, 1, 7},
        {Gamepad_B, 1, 6},         {Gamepad_Y, 1, 5},            {Gamepad_A, 1, 4},
        {Gamepad_X, 1, 3},         {Gamepad_RightShoulder, 1, 2}, {Gamepad_DpadLeft, 1, 1},
        {Gamepad_DpadUp, 1, 0},
    };
    const uint8_t first_button_byte[] = {0, 4, 7, 6};
    for (uint8_t format : {1, 2, 3})
    {
        for (const auto &b : buttons)
        {
            mappings.clear();
            auto idle = report(Gamepad, format);
            pressed<GamepadButtonMapping>(button(b.button));
            EXPECT_EQ(report(Gamepad, format), with_cleared(idle, first_button_byte[format] + b.byte, b.bit))
                << "format " << int(format) << " button " << int(b.button);
        }
    }
}

TEST_F(WiiReport, ClassicFormat1FullDeflection)
{
    at<GamepadAxisMapping>(with_gamepad_axis(stick_config(), Gamepad_LeftStickX), UINT16_MAX);
    at<GamepadAxisMapping>(with_gamepad_axis(stick_config(), Gamepad_RightStickX), UINT16_MAX);
    at<GamepadAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_RightTrigger), UINT16_MAX);
    // LX 63, LY 32, RX 31, RY 16, LT 0, RT 31 (and the RT click)
    EXPECT_EQ(report(Gamepad, 1), (std::vector<uint8_t>{0xFF, 0xE0, 0x90, 0x1F, 0xFD, 0xFF}));
}

TEST_F(WiiReport, ClassicFormat3FullDeflection)
{
    at<GamepadAxisMapping>(with_gamepad_axis(stick_config(), Gamepad_LeftStickY), UINT16_MAX);
    at<GamepadAxisMapping>(with_gamepad_axis(stick_config(), Gamepad_RightStickX), 0);
    at<GamepadAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), UINT16_MAX);
    // LX 80, RX 00, LY FF, RY 80, LT FF and its click, RT 0
    EXPECT_EQ(report(Gamepad, 3), (std::vector<uint8_t>{0x80, 0x00, 0xFF, 0x80, 0xFF, 0x00, 0xDF, 0xFF}));
}

// Guitar Hero guitar: 6 byte report, buttons 4: 1 BD 1 B- 1 B+ 1 1, 5: BO BR BB BG BY PB 1 BU

TEST_F(WiiReport, GuitarNeutralSticksAndButtons)
{
    auto r = report(GuitarHeroGuitar);
    ASSERT_EQ(r.size(), 6u);
    EXPECT_EQ(r[0] & 0x3F, 0x20);
    EXPECT_EQ(r[1] & 0x3F, 0x20);
    EXPECT_EQ(r[2] & 0x1F, 0x0F); // touch bar not touched
    EXPECT_EQ(r[3] & 0x1F, 0x00); // whammy at rest
    EXPECT_EQ(r[4], 0xFF);
    EXPECT_EQ(r[5], 0xFF);
}

TEST_F(WiiReport, GuitarFretsAndPedal)
{
    const struct
    {
        GuitarHeroGuitarButtonType button;
        uint8_t bit;
    } frets[] = {
        {GuitarHeroGuitar_Orange, 7}, {GuitarHeroGuitar_Red, 6},    {GuitarHeroGuitar_Blue, 5},
        {GuitarHeroGuitar_Green, 4},  {GuitarHeroGuitar_Yellow, 3}, {GuitarHeroGuitar_Pedal, 2},
    };
    auto idle = report(GuitarHeroGuitar);
    for (const auto &f : frets)
    {
        mappings.clear();
        pressed<GuitarHeroGuitarButtonMapping>(gh_button(f.button));
        EXPECT_EQ(report(GuitarHeroGuitar), with_cleared(idle, 5, f.bit)) << int(f.button);
    }
}

TEST_F(WiiReport, GuitarStrumAndPlusMinus)
{
    const struct
    {
        GamepadButtonType button;
        uint8_t byte;
        uint8_t bit;
    } buttons[] = {
        {Gamepad_DpadUp, 5, 0}, {Gamepad_DpadDown, 4, 6}, {Gamepad_Start, 4, 2}, {Gamepad_Back, 4, 4},
    };
    auto idle = report(GuitarHeroGuitar);
    for (const auto &b : buttons)
    {
        mappings.clear();
        pressed<GamepadButtonMapping>(button(b.button));
        EXPECT_EQ(report(GuitarHeroGuitar), with_cleared(idle, b.byte, b.bit)) << int(b.button);
    }
}

TEST_F(WiiReport, GuitarWhammyIsFiveBitInByteThree)
{
    at<GuitarHeroGuitarAxisMapping>(gh_axis(GuitarHeroGuitar_Whammy), UINT16_MAX);
    EXPECT_EQ(report(GuitarHeroGuitar)[3] & 0x1F, 0x1F);
}

// wiibrew's Guitar Hero guitar page gives the touch bar codes a guitar actually sends: 1st (green)
// 04, 2nd 0A, 3rd 12, 4th 17, 5th 1F. GuitarHeroGuitarButtonMapping::update_wii
// (src/mappings/red_octane_mappings.cpp) sends those, which is also what the firmware's own decoder
// (wii_extension_decoder.cpp) expects.
TEST_F(WiiReport, TouchBarFretsUseTheGuitarsCodes)
{
    const struct
    {
        GuitarHeroGuitarButtonType button;
        uint8_t code;
    } taps[] = {
        {GuitarHeroGuitar_TapGreen, 0x04}, {GuitarHeroGuitar_TapRed, 0x0A},  {GuitarHeroGuitar_TapYellow, 0x12},
        {GuitarHeroGuitar_TapBlue, 0x17},  {GuitarHeroGuitar_TapOrange, 0x1F},
    };
    for (const auto &t : taps)
    {
        mappings.clear();
        pressed<GuitarHeroGuitarButtonMapping>(gh_button(t.button));
        EXPECT_EQ(report(GuitarHeroGuitar)[2] & 0x1F, t.code) << int(t.button);
    }
}

// wiibrew: two adjacent frets have their own codes (07, 0C, 14, 1A); frets that aren't next to each
// other can't be shown, so the higher one is sent
TEST_F(WiiReport, TouchBarChordsUseTheGuitarsCodes)
{
    const struct
    {
        GuitarHeroGuitarButtonType first, second;
        uint8_t code;
    } chords[] = {
        {GuitarHeroGuitar_TapGreen, GuitarHeroGuitar_TapRed, 0x07},
        {GuitarHeroGuitar_TapYellow, GuitarHeroGuitar_TapRed, 0x0C},
        {GuitarHeroGuitar_TapYellow, GuitarHeroGuitar_TapBlue, 0x14},
        {GuitarHeroGuitar_TapOrange, GuitarHeroGuitar_TapBlue, 0x1A},
        {GuitarHeroGuitar_TapGreen, GuitarHeroGuitar_TapYellow, 0x12},
    };
    for (const auto &c : chords)
    {
        mappings.clear();
        pressed<GuitarHeroGuitarButtonMapping>(gh_button(c.first));
        pressed<GuitarHeroGuitarButtonMapping>(gh_button(c.second));
        EXPECT_EQ(report(GuitarHeroGuitar)[2] & 0x1F, c.code) << int(c.first) << " " << int(c.second);
    }
}

// GHWT drums: 5: O R Y G B Bass 1 1, all active low

TEST_F(WiiReport, DrumNeutralButtons)
{
    auto r = report(GuitarHeroDrums);
    ASSERT_EQ(r.size(), 6u);
    EXPECT_EQ(r[0] & 0x3F, 0x20);
    EXPECT_EQ(r[1] & 0x3F, 0x20);
    EXPECT_EQ(r[4], 0xFF);
    EXPECT_EQ(r[5], 0xFF);
}

// Everything in the drum report is inverted, and wiibrew: "Since the data is inverted, bytes 2 and 3
// will be FF FF if there is no data present". initialize_wii_extension_report
// (src/emulation/wii_extension_input.cpp) starts them at FF FF; 00 00 would decode as note 0x7F with
// a velocity.
TEST_F(WiiReport, DrumNeutralSendsNoMidiNote)
{
    auto r = report(GuitarHeroDrums);
    EXPECT_EQ(r[2], 0xFF);
    EXPECT_EQ(r[3], 0xFF);
}

TEST_F(WiiReport, DrumPadsAreActiveLow)
{
    const struct
    {
        GuitarHeroDrumsAxisType pad;
        uint8_t bit;
    } pads[] = {
        {GuitarHeroDrums_RedPad, 6}, {GuitarHeroDrums_YellowPad, 5}, {GuitarHeroDrums_GreenPad, 4},
        {GuitarHeroDrums_BluePad, 3},
    };
    auto idle = report(GuitarHeroDrums);
    for (const auto &p : pads)
    {
        mappings.clear();
        at<GuitarHeroDrumsAxisMapping>(gh_drum(p.pad), UINT16_MAX);
        EXPECT_EQ(report(GuitarHeroDrums), with_cleared(idle, 5, p.bit)) << int(p.pad);
    }
}

// wiibrew's drum table has byte 5 = O R Y G B Bass 1 1: orange is bit 7 and the bass pedal bit 2
// (Dolphin's Drums.h agrees: PAD_ORANGE 0x80, PAD_BASS 0x04). GuitarHeroDrumsAxisMapping::update_wii
// (src/mappings/red_octane_mappings.cpp) sets leftShoulder / rightShoulder of WiiDrumDataFormat3_t.
TEST_F(WiiReport, DrumOrangeAndKickBits)
{
    auto idle = report(GuitarHeroDrums);
    at<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_OrangePad), UINT16_MAX);
    EXPECT_EQ(report(GuitarHeroDrums), with_cleared(idle, 5, 7));
    mappings.clear();
    at<GuitarHeroDrumsAxisMapping>(gh_drum(GuitarHeroDrums_KickPedal), UINT16_MAX);
    EXPECT_EQ(report(GuitarHeroDrums), with_cleared(idle, 5, 2));
}

TEST_F(WiiReport, DrumPlusMinus)
{
    auto idle = report(GuitarHeroDrums);
    pressed<GamepadButtonMapping>(button(Gamepad_Start));
    EXPECT_EQ(report(GuitarHeroDrums), with_cleared(idle, 4, 2));
    mappings.clear();
    pressed<GamepadButtonMapping>(button(Gamepad_Back));
    EXPECT_EQ(report(GuitarHeroDrums), with_cleared(idle, 4, 4));
}

// DJ Hero turntable
//   0: RTT<4:3> SX   1: RTT<2:1> SY   2: RTT<0> ED<4:3> CS<3:0> RTT<5>   3: ED<2:0> LTT<4:0>
//   4: 0 0 LBR B- 0 B+ RBR LTT<5>    5: LBB 0 RBG BE LBG RBB 0 0

namespace
{
int left_turntable(const std::vector<uint8_t> &r)
{
    int v = (r[3] & 0x1F) | ((r[4] & 1) << 5);
    return v >= 32 ? v - 64 : v;
}
int right_turntable(const std::vector<uint8_t> &r)
{
    int v = ((r[0] >> 6) << 3) | ((r[1] >> 6) << 1) | (r[2] >> 7) | ((r[2] & 1) << 5);
    return v >= 32 ? v - 64 : v;
}
} // namespace

TEST_F(WiiReport, TurntableIdleRightTurntableIsStill)
{
    auto r = report(DjHeroTurntable);
    ASSERT_EQ(r.size(), 6u);
    EXPECT_EQ(r[0] & 0x3F, 0x20);
    EXPECT_EQ(r[1] & 0x3F, 0x20);
    EXPECT_EQ(right_turntable(r), 0);
}

// LTT<5>, the left turntable's sign bit, is bit 0 of byte 4, which WiiTurntableDataFormat3_t
// (include/protocols/wii.hpp) puts in buttonsLow. finalize_wii_extension_report
// (src/emulation/wii_extension_input.cpp) inverts the rest of that byte to make the buttons active
// low but leaves LTT<5> alone, so the turntables come out as wiibrew's plain 6 bit signed values
// (which is how the firmware's own decoder reads them).
TEST_F(WiiReport, TurntableLeftVelocityKeepsItsSign)
{
    EXPECT_EQ(left_turntable(report(DjHeroTurntable)), 0);
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_LeftVelocity), 0x8000 + 5 * 1024);
    EXPECT_EQ(left_turntable(report(DjHeroTurntable)), 5);
}

TEST_F(WiiReport, TurntableRightVelocityIsSignedSixBit)
{
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_RightVelocity), 0x8000 + 5 * 1024);
    EXPECT_EQ(right_turntable(report(DjHeroTurntable)), 5);
    mappings.clear();
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_RightVelocity), 0x8000 - 7 * 1024);
    EXPECT_EQ(right_turntable(report(DjHeroTurntable)), -7);
}

TEST_F(WiiReport, TurntableCrossfaderAndEffectsDial)
{
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_Crossfader), UINT16_MAX);
    at<DJHTurntableAxisMapping>(djh_axis(DJHTurntable_EffectsKnob), UINT16_MAX);
    auto r = report(DjHeroTurntable);
    EXPECT_EQ((r[2] >> 1) & 0xF, 0xF);
    EXPECT_EQ(((r[2] >> 5) & 3) << 3 | (r[3] >> 5), 31);
    EXPECT_EQ(right_turntable(r), 0);
}

TEST_F(WiiReport, TurntablePlusMinus)
{
    auto idle = report(DjHeroTurntable);
    pressed<GamepadButtonMapping>(button(Gamepad_Start));
    EXPECT_EQ(report(DjHeroTurntable), with_cleared(idle, 4, 2));
    mappings.clear();
    pressed<GamepadButtonMapping>(button(Gamepad_Back));
    EXPECT_EQ(report(DjHeroTurntable), with_cleared(idle, 4, 4));
}

// DJHTurntableButtonMapping::update_wii (src/mappings/red_octane_mappings.cpp) writes through the
// 6 byte WiiTurntableDataFormat3_t, so the buttons land where wiibrew has them:
// 4: .. LBR .. RBR .., 5: LBB 0 RBG BE LBG RBB.
TEST_F(WiiReport, TurntableButtonsReachTheReport)
{
    const struct
    {
        DJHTurntableButtonType button;
        uint8_t byte;
        uint8_t bit;
    } buttons[] = {
        {DJHTurntable_LeftGreen, 5, 3},  {DJHTurntable_LeftRed, 4, 5},  {DJHTurntable_LeftBlue, 5, 7},
        {DJHTurntable_RightGreen, 5, 5}, {DJHTurntable_RightRed, 4, 1}, {DJHTurntable_RightBlue, 5, 2},
    };
    auto idle = report(DjHeroTurntable);
    for (const auto &b : buttons)
    {
        mappings.clear();
        pressed<DJHTurntableButtonMapping>(djh_button(b.button));
        EXPECT_EQ(report(DjHeroTurntable), with_cleared(idle, b.byte, b.bit)) << int(b.button);
    }
}

// TaTaCon

TEST_F(WiiReport, TaikoNeutral)
{
    EXPECT_EQ(report(Taiko), (std::vector<uint8_t>{0xA0, 0x20, 0x50, 0x10, 0xFF, 0xFF}));
}

TEST_F(WiiReport, TaikoHitsClearTheirBits)
{
    // byte 5: 1 CL RL CR RR 1 1 1
    const struct
    {
        GamepadButtonType button;
        uint8_t bit;
    } hits[] = {
        {Gamepad_DpadDown, 6}, // left centre
        {Gamepad_DpadLeft, 5}, // left rim
        {Gamepad_A, 4},        // right centre
        {Gamepad_B, 3},        // right rim
    };
    auto idle = report(Taiko);
    for (const auto &h : hits)
    {
        mappings.clear();
        pressed<TaikoButtonMapping>(button(h.button));
        EXPECT_EQ(report(Taiko), with_cleared(idle, 5, h.bit)) << int(h.button);
    }
}
