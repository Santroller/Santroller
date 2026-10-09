#include <gtest/gtest.h>
#include <cstring>
#include "emulation/usb/spice2x_device.h"
#include "firmware_descriptors.hpp"
#include "protocols/hid.hpp"
#include "protocols/xinput.hpp"
#include "struct_fields.hpp"

// Arcade emulation modes.
//
// Pairing, from the code that sends the reports:
//  - GHArcadeGamepadDevice (ModeGuitarHeroArcade, gh_arcade_device.cpp) fills epin_buf as an
//    ArcadeGuitarHeroGuitar_Data_t (no report id) and sends sizeof(XInputGamepad_Data_t) bytes of it.
//  - Spice2xDevice (ModeSpice2x, spice2x_device.cpp, built here from the real source) sends
//    sizeof(m_input), a Spice2xInputReport with the report id in the struct. Output reports 2 and 3 are
//    the panel / status LEDs, each named by a string index patched in by initialize().
//  - PDLoaderDevice (ModePdLoader) is a vendor class interface with no HID report descriptor.

using namespace hid_desc;

// ---------------------------------------------------------------------------------------------
// Guitar Hero Arcade

TEST(GhArcadeDescriptor, ReportSizeMatchesTheStruct)
{
    auto d = parse(kGhArcade);
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_FALSE(d.uses_report_ids);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, 0), sizeof(ArcadeGuitarHeroGuitar_Data_t));
}

TEST(GhArcadeDescriptor, FieldsMatchTheStruct)
{
    auto d = parse(kGhArcade);
    ASSERT_TRUE(d.ok) << d.error;
    const ReportType in = ReportType::Input;
    // five bytes of axes, the last one the tilt
    expect_field(d, in, 0, desktop(kDesktopX), FIELD(ArcadeGuitarHeroGuitar_Data_t, always_1d), false);
    expect_field(d, in, 0, desktop(kDesktopY), FIELD(ArcadeGuitarHeroGuitar_Data_t, unk1), false);
    expect_field(d, in, 0, desktop(kDesktopZ), FIELD(ArcadeGuitarHeroGuitar_Data_t, unk2), false);
    expect_field(d, in, 0, desktop(kDesktopRx), FIELD(ArcadeGuitarHeroGuitar_Data_t, always_ff), false);
    expect_field(d, in, 0, desktop(kDesktopRy), FIELD(ArcadeGuitarHeroGuitar_Data_t, tilt), false);
    // the dpad nibble is the hat, then the frets are buttons 1 - 5
    const Element *hat = d.find(in, 0, desktop(kDesktopHat));
    ASSERT_NE(hat, nullptr);
    EXPECT_EQ(hat->bit_offset, FIELD(ArcadeGuitarHeroGuitar_Data_t, dpad).bit);
    EXPECT_EQ(hat->bit_size, FIELD(ArcadeGuitarHeroGuitar_Data_t, dpad).width);
    expect_field(d, in, 0, button(1), FIELD(ArcadeGuitarHeroGuitar_Data_t, a), false);
    expect_field(d, in, 0, button(2), FIELD(ArcadeGuitarHeroGuitar_Data_t, b), false);
    expect_field(d, in, 0, button(3), FIELD(ArcadeGuitarHeroGuitar_Data_t, x), false);
    expect_field(d, in, 0, button(4), FIELD(ArcadeGuitarHeroGuitar_Data_t, y), false);
    expect_field(d, in, 0, button(5), FIELD(ArcadeGuitarHeroGuitar_Data_t, leftShoulder), false);
    // the cabinet side (1 left, 2 right) is buttons 9 and 10
    const FieldBits side = FIELD(ArcadeGuitarHeroGuitar_Data_t, side);
    const Element *b9 = d.find(in, 0, button(9));
    const Element *b10 = d.find(in, 0, button(10));
    ASSERT_TRUE(b9 && b10);
    EXPECT_EQ(b9->bit_offset, side.bit);
    EXPECT_EQ(b10->bit_offset, side.bit + 1);
}

// GHArcadeGamepadDevice::process (gh_arcade_device.cpp:145) sends sizeof(XInputGamepad_Data_t) = 20
// bytes, but its report is an ArcadeGuitarHeroGuitar_Data_t and the descriptor (no report ids) declares
// a 7 byte input report (TUD_HID_REPORT_DESC_GUITAR_HERO_ARCADE, include/hid_reports.h:132-171). Every
// report carries 13 undeclared trailing zero bytes. Hosts that use the raw bytes don't care; hosts that
// check report lengths may reject or truncate them.
TEST(GhArcadeDescriptor, DISABLED_SentLengthMatchesTheDescriptor)
{
    auto d = parse(kGhArcade);
    ASSERT_TRUE(d.ok) << d.error;
    const size_t sent = sizeof(XInputGamepad_Data_t);
    EXPECT_EQ(sent, d.wire_bytes(ReportType::Input, 0));
}

// ---------------------------------------------------------------------------------------------
// Spice2x

namespace
{
Descriptor spice2x()
{
    auto d = parse(spice2x_descriptor());
    EXPECT_TRUE(d.ok) << d.error;
    return d;
}
} // namespace

TEST(Spice2xDescriptor, ReportSizes)
{
    auto d = spice2x();
    EXPECT_EQ(d.wire_bytes(ReportType::Input, SPICE2X_INPUT_ID), sizeof(Spice2xInputReport));
    // one byte per LED: nine panel LEDs, three status LEDs
    EXPECT_EQ(d.report_bytes(ReportType::Output, SPICE2X_PANEL_LED_ID), 9u);
    EXPECT_EQ(d.report_bytes(ReportType::Output, SPICE2X_STATUS_LED_ID), 3u);
}

TEST(Spice2xDescriptor, FieldsMatchWhatProcessWrites)
{
    auto d = spice2x();
    const ReportType in = ReportType::Input;
    const uint8_t id = SPICE2X_INPUT_ID;
    // process() packs the hat into bits 0 - 3 of controller and buttons from bit 4 (a << 4 ... capture << 13)
    const FieldBits controller = FIELD(Spice2xInputReport, controller);
    const FieldBits pad = FIELD(Spice2xInputReport, pad);
    const Element *hat = d.find(in, id, desktop(kDesktopHat));
    ASSERT_NE(hat, nullptr);
    EXPECT_EQ(hat->bit_offset + 8, controller.bit);
    EXPECT_EQ(hat->bit_size, 4u);
    EXPECT_TRUE(hat->null_state());
    for (uint16_t n = 1; n <= 10; n++)
    {
        const Element *e = d.find(in, id, button(n));
        ASSERT_NE(e, nullptr) << n;
        EXPECT_EQ(e->bit_offset + 8, controller.bit + 3 + n) << "button " << n;
    }
    // and the panel into pad: start, back, then the nine arrows (Spice2xPad, bits 2 - 10)
    const uint16_t pad_bits[] = {1, 2, SpiceUpLeft, SpiceUp, SpiceUpRight, SpiceLeft, SpiceCenter,
                                 SpiceRight, SpiceDownLeft, SpiceDown, SpiceDownRight};
    for (uint16_t n = 11; n <= 21; n++)
    {
        const Element *e = d.find(in, id, button(n));
        ASSERT_NE(e, nullptr) << n;
        EXPECT_EQ(uint32_t(1) << (e->bit_offset + 8 - pad.bit), pad_bits[n - 11]) << "button " << n;
    }
    // the hat table maps every dpad combination to 0 - 8
    EXPECT_EQ(hat->logical_max, 7);
}

TEST(Spice2xDescriptor, LedStringIndicesNameTheLeds)
{
    Spice2xDevice device;
    device.initialize();
    auto d = parse(device.report_descriptor(), device.report_desc_len());
    ASSERT_TRUE(d.ok) << d.error;
    // initialize() rewrites every String Index (0x79) item: they must be the LED outputs, in order
    std::vector<uint32_t> indices;
    for (const auto &item : d.items)
    {
        if (item.string_indices.empty())
            continue;
        EXPECT_EQ(item.type, ReportType::Output);
        indices.insert(indices.end(), item.string_indices.begin(), item.string_indices.end());
    }
    ASSERT_EQ(indices.size(), 12u);
    for (size_t i = 0; i < indices.size(); i++)
    {
        EXPECT_EQ(indices[i], device.m_led_strid + i);
        char name[64] = {};
        EXPECT_GT(device.device_name(uint8_t(indices[i]), name), 1u) << "string " << indices[i];
    }
}
