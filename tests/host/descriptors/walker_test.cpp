#include <gtest/gtest.h>
#include <vector>
#include "hid_descriptor.hpp"

// The descriptor walker the other descriptor tests rely on, checked on small descriptors whose layout
// follows directly from the HID 1.11 item encoding (6.2.2).

using namespace hid_desc;
using Bytes = std::vector<uint8_t>;

namespace
{
// Report 1: 12 buttons, a 4 bit hat (0 - 7, null state), X / Y unsigned 8 bit, Z / Rz signed 8 bit
const Bytes kGamepad = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x0C, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0C, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42,
    0x09, 0x30, 0x09, 0x31, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
    0x09, 0x32, 0x09, 0x35, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
    0xC0};

// wraps report items in an application collection
Bytes app(const Bytes &items)
{
    Bytes out = {0x05, 0x01, 0x09, 0x05, 0xA1, 0x01};
    out.insert(out.end(), items.begin(), items.end());
    out.push_back(0xC0);
    return out;
}
} // namespace

TEST(DescriptorWalker, GamepadLayout)
{
    auto d = parse(kGamepad);
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_TRUE(d.uses_report_ids);
    // 12 + 4 + 16 + 16 bits
    EXPECT_EQ(d.report_bytes(ReportType::Input, 1), 6u);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, 1), 7u);
    EXPECT_EQ(d.reports().size(), 1u);

    const Element *b1 = d.find(ReportType::Input, 1, button(1));
    const Element *b12 = d.find(ReportType::Input, 1, button(12));
    ASSERT_TRUE(b1 && b12);
    EXPECT_EQ(b1->bit_offset, 0u);
    EXPECT_EQ(b12->bit_offset, 11u);
    EXPECT_EQ(d.find(ReportType::Input, 1, button(13)), nullptr);

    const Element *hat = d.find(ReportType::Input, 1, desktop(kDesktopHat));
    ASSERT_NE(hat, nullptr);
    EXPECT_EQ(hat->bit_offset, 12u);
    EXPECT_EQ(hat->bit_size, 4u);
    EXPECT_TRUE(hat->null_state());
    EXPECT_EQ(hat->logical_max, 7);

    const Element *x = d.find(ReportType::Input, 1, desktop(kDesktopX));
    const Element *rz = d.find(ReportType::Input, 1, desktop(kDesktopRz));
    ASSERT_TRUE(x && rz);
    EXPECT_EQ(x->bit_offset, 16u);
    EXPECT_EQ(x->logical_max, 255); // 2 byte item, so not sign extended
    EXPECT_EQ(rz->bit_offset, 40u);
    EXPECT_EQ(rz->logical_min, -127); // 1 byte 0x81, sign extended
    EXPECT_TRUE(overlaps(d).empty());
}

TEST(DescriptorWalker, LastUsageRepeatsForTheRestOfTheCount)
{
    // usages X, Y for a count of 4: the last two are Y as well (HID 1.11 6.2.2.8)
    auto d = parse(app({0x09, 0x30, 0x09, 0x31, 0x75, 0x08, 0x95, 0x04, 0x81, 0x02}));
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_EQ(d.find_all(ReportType::Input, 0, desktop(kDesktopY)).size(), 3u);
    EXPECT_FALSE(d.uses_report_ids);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, 0), 4u);
}

TEST(DescriptorWalker, ExtendedUsagesCarryTheirOwnPage)
{
    // usage page vendor, then a 4 byte usage on the desktop page
    auto d = parse(app({0x06, 0x00, 0xFF, 0x0B, 0x30, 0x00, 0x01, 0x00, 0x75, 0x08, 0x95, 0x01, 0x81, 0x02}));
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_NE(d.find(ReportType::Input, 0, desktop(kDesktopX)), nullptr);
}

TEST(DescriptorWalker, ReportTypesAndIdsAreCountedSeparately)
{
    auto d = parse(app({0x85, 0x01, 0x75, 0x08, 0x95, 0x02, 0x09, 0x30, 0x81, 0x02,  // input 1: 2 bytes
                        0x95, 0x03, 0x09, 0x30, 0x91, 0x02,                          // output 1: 3 bytes
                        0x85, 0x02, 0x95, 0x05, 0x09, 0x30, 0xB1, 0x02}));           // feature 2: 5 bytes
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_EQ(d.report_bytes(ReportType::Input, 1), 2u);
    EXPECT_EQ(d.report_bytes(ReportType::Output, 1), 3u);
    EXPECT_EQ(d.report_bytes(ReportType::Feature, 2), 5u);
    EXPECT_FALSE(d.has_report(ReportType::Input, 2));
    auto outputs = d.find_all(ReportType::Output, 1, desktop(kDesktopX));
    ASSERT_EQ(outputs.size(), 3u);
    EXPECT_EQ(outputs[0]->bit_offset, 0u);
    EXPECT_EQ(outputs[2]->bit_offset, 16u);
}

TEST(DescriptorWalker, PushAndPopRestoreGlobals)
{
    // report size 8, push, report size 16, pop: the input is 8 bit again
    auto d = parse(app({0x75, 0x08, 0x95, 0x01, 0xA4, 0x75, 0x10, 0xB4, 0x09, 0x30, 0x81, 0x02}));
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_EQ(d.find(ReportType::Input, 0, desktop(kDesktopX))->bit_size, 8u);
}

TEST(DescriptorWalker, ArrayItemsHaveNoPositionalUsages)
{
    auto d = parse(app({0x05, 0x07, 0x19, 0x00, 0x29, 0xFF, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x06, 0x81, 0x00}));
    ASSERT_TRUE(d.ok) << d.error;
    ASSERT_EQ(d.items.size(), 1u);
    EXPECT_TRUE(d.items[0].has_range);
    EXPECT_EQ(d.items[0].usage_max, usage(kPageKeyboard, 0xFF));
    for (const auto &e : d.elements)
        EXPECT_EQ(e.usage, 0u);
}

TEST(DescriptorWalker, RejectsBrokenDescriptors)
{
    struct Case
    {
        const char *what;
        Bytes desc;
    };
    const Case cases[] = {
        {"truncated item", {0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x26, 0xFF}},
        {"unclosed collection", {0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x75, 0x08, 0x95, 0x01, 0x81, 0x03}},
        {"extra end collection", app({0xC0})},
        {"pop without push", app({0xB4})},
        {"push without pop", app({0xA4})},
        {"report id 0", app({0x85, 0x00, 0x75, 0x08, 0x95, 0x01, 0x81, 0x03})},
        {"main item before the first report id", app({0x75, 0x08, 0x95, 0x01, 0x81, 0x03, 0x85, 0x01, 0x81, 0x03})},
        {"logical minimum above maximum", app({0x15, 0x05, 0x25, 0x01, 0x75, 0x08, 0x95, 0x01, 0x09, 0x30, 0x81, 0x02})},
        {"usage minimum without maximum", app({0x19, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02})},
        {"no report size", app({0x95, 0x01, 0x81, 0x03})},
        {"long item", app({0xFE, 0x00, 0x00})},
        {"no application collection", {0x75, 0x08, 0x95, 0x01, 0x81, 0x03}},
    };
    for (const auto &c : cases)
    {
        auto d = parse(c.desc);
        EXPECT_FALSE(d.ok) << c.what;
    }
}

TEST(DescriptorWalker, FindsOverlaps)
{
    // nothing in a well formed descriptor can overlap, so check the helper on a hand built element list
    Descriptor d;
    Element a{};
    a.type = ReportType::Input;
    a.bit_offset = 0;
    a.bit_size = 8;
    Element b = a;
    b.bit_offset = 4;
    d.elements = {a, b};
    EXPECT_EQ(overlaps(d).size(), 1u);
}
