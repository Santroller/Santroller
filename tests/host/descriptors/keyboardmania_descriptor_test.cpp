#include <gtest/gtest.h>
#include <cstring>
#include <set>
#include "hid_descriptor.hpp"
#include "protocols/keyboardmania.hpp"
#include "struct_fields.hpp"

// KeyboardManiaDevice (keyboardmania_device.cpp) sends sizeof(m_input), a KeyboardManiaInputReport with no
// report id, whose buttons are set from keyboardmania_bits by KeyboardManiaButtonMapping and ProKeysKeyMapping.

using namespace hid_desc;

TEST(KeyboardManiaDescriptor, ReportSizeMatchesTheStruct)
{
    auto d = parse(keyboardmania_report_descriptor);
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_FALSE(d.uses_report_ids);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, 0), sizeof(KeyboardManiaInputReport));
}

TEST(KeyboardManiaDescriptor, HeaderIsTheAlwaysSetButtons)
{
    auto d = parse(keyboardmania_report_descriptor);
    ASSERT_TRUE(d.ok) << d.error;
    for (uint16_t n = 58; n <= 63; n++)
    {
        const Element *e = d.find(ReportType::Input, 0, button(n));
        ASSERT_NE(e, nullptr) << n;
        EXPECT_EQ(e->bit_offset, n - 58u);
    }
    EXPECT_EQ(KEYBOARDMANIA_HEADER, 0x3f);
}

// Every output lands on a button the descriptor declares, and no two share one
TEST(KeyboardManiaDescriptor, BitsAreDescriptorButtons)
{
    auto d = parse(keyboardmania_report_descriptor);
    ASSERT_TRUE(d.ok) << d.error;
    const uint32_t buttons = FIELD(KeyboardManiaInputReport, buttons).bit;
    std::set<uint32_t> declared;
    for (uint16_t n = 1; n <= 28; n++)
    {
        const Element *e = d.find(ReportType::Input, 0, button(n));
        ASSERT_NE(e, nullptr) << n;
        declared.insert(e->bit_offset - buttons);
    }
    std::set<uint32_t> used(std::begin(keyboardmania_bits), std::end(keyboardmania_bits));
    EXPECT_EQ(used.size(), sizeof(keyboardmania_bits));
    EXPECT_EQ(used, declared);
}

// The bitmasks the keyboardmania-midi-bridge README lists for bytes 2 - 5 of a report
TEST(KeyboardManiaDescriptor, MatchesTheRealController)
{
    struct Case
    {
        size_t output; // KeyboardManiaButtonType - 1
        uint8_t bytes[4];
    };
    const Case cases[] = {
        {0, {0x01, 0x00, 0x00, 0x00}},  // C3
        {6, {0x40, 0x00, 0x00, 0x00}},  // F#3
        {7, {0x00, 0x01, 0x00, 0x00}},  // G3
        {12, {0x00, 0x20, 0x00, 0x00}}, // C4
        {13, {0x00, 0x00, 0x01, 0x00}}, // C#4
        {18, {0x00, 0x00, 0x20, 0x00}}, // F#4
        {19, {0x00, 0x00, 0x00, 0x01}}, // G4
        {23, {0x00, 0x00, 0x00, 0x10}}, // B4
        {24, {0x00, 0x00, 0x40, 0x00}}, // Start
        {25, {0x00, 0x40, 0x00, 0x00}}, // Select
        {26, {0x00, 0x00, 0x00, 0x20}}, // Wheel Up
        {27, {0x00, 0x00, 0x00, 0x40}}, // Wheel Down
    };
    for (const auto &c : cases)
    {
        KeyboardManiaInputReport report = {KEYBOARDMANIA_HEADER, 0, 0, {}};
        report.buttons |= 1u << keyboardmania_bits[c.output];
        uint8_t raw[sizeof(report)];
        memcpy(raw, &report, sizeof(report));
        EXPECT_EQ(raw[0], 0x3f);
        EXPECT_EQ(memcmp(raw + 1, c.bytes, 4), 0) << "output " << c.output;
        EXPECT_EQ(raw[5] | raw[6] | raw[7], 0);
    }
}
