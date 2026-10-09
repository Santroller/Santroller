#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include "hidparser.h"

namespace
{
using Bytes = std::vector<uint8_t>;

// Report 1: 12 buttons, 4 bit hat (0 - 7, neutral outside), X / Y unsigned 8 bit, Z / Rz signed 8 bit
const Bytes kGamepad = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x0C, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0C, 0x81, 0x02,
    0x05, 0x01, 0x09, 0x39, 0x15, 0x00, 0x25, 0x07, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42,
    0x09, 0x30, 0x09, 0x31, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
    0x09, 0x32, 0x09, 0x35, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x02,
    0xC0};

// RAII so a failing assertion doesn't leak the parser's shared pools into later tests
struct Parsed
{
    HID_ReportInfo_t *info = nullptr;
    uint8_t result;
    explicit Parsed(const Bytes &desc) { result = USB_ProcessHIDReport(desc.data(), desc.size(), &info); }
    ~Parsed() { USB_FreeReportInfo(info); }
};

USB_Host_Data_t fill(HID_ReportInfo_t *info, const Bytes &report, USB_Host_Data_t data = {})
{
    fill_generic_report(info, report.data(), &data);
    return data;
}

// buttons low then high byte, hat in the low nibble of the next byte, then X, Y, Z, Rz
Bytes gamepad_report(uint16_t buttons, uint8_t hat, uint8_t x, uint8_t y, int8_t z, int8_t rz)
{
    return {0x01, (uint8_t)buttons, (uint8_t)(((buttons >> 8) & 0x0F) | ((hat & 0x0F) << 4)), x, y, (uint8_t)z, (uint8_t)rz};
}
} // namespace

TEST(HidParser, GamepadDescriptorParses)
{
    Parsed parsed(kGamepad);
    ASSERT_EQ(parsed.result, HID_PARSE_Successful);
    ASSERT_NE(parsed.info, nullptr);
    EXPECT_TRUE(parsed.info->UsingReportIDs);
    // 12 buttons, the hat and four axes
    EXPECT_EQ(parsed.info->TotalReportItems, 12 + 1 + 4);
}

TEST(HidParser, KeyboardOnlyHasNothingForTheGenericMapper)
{
    const Bytes keyboard = {
        0x05, 0x01, 0x09, 0x06, 0xA1, 0x01,
        0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x26, 0xFF, 0x00, 0x05, 0x07, 0x19, 0x00, 0x2A, 0xFF, 0x00, 0x81, 0x00,
        0xC0};
    Parsed parsed(keyboard);
    EXPECT_EQ(parsed.result, HID_PARSE_NoUnfilteredReportItems);
}

TEST(HidParser, ButtonsMapToBits)
{
    Parsed parsed(kGamepad);
    auto data = fill(parsed.info, gamepad_report(0x0801, 0x0F, 0x80, 0x80, 0, 0));
    EXPECT_EQ(data.genericButtons, 0x0801);
}

TEST(HidParser, HatDirections)
{
    Parsed parsed(kGamepad);
    struct Case
    {
        uint8_t hat;
        bool up, right, down, left;
    };
    const Case cases[] = {
        {0, true, false, false, false},
        {1, true, true, false, false},
        {2, false, true, false, false},
        {3, false, true, true, false},
        {4, false, false, true, false},
        {5, false, false, true, true},
        {6, false, false, false, true},
        {7, true, false, false, true},
        {8, false, false, false, false}, // outside the logical range is neutral
        {15, false, false, false, false},
    };
    for (const auto &c : cases)
    {
        SCOPED_TRACE(testing::Message() << "hat " << (int)c.hat);
        auto data = fill(parsed.info, gamepad_report(0, c.hat, 0x80, 0x80, 0, 0));
        EXPECT_EQ((bool)data.dpadUp, c.up);
        EXPECT_EQ((bool)data.dpadRight, c.right);
        EXPECT_EQ((bool)data.dpadDown, c.down);
        EXPECT_EQ((bool)data.dpadLeft, c.left);
    }
}

TEST(HidParser, UnsignedAxesScaleToTheFullRange)
{
    Parsed parsed(kGamepad);
    auto low = fill(parsed.info, gamepad_report(0, 8, 0x00, 0xFF, 0, 0));
    EXPECT_EQ(low.genericAxisX, 0);
    EXPECT_EQ(low.genericAxisY, UINT16_MAX);
    EXPECT_TRUE(low.genericAxesPresent & GENERIC_AXIS_X);
    EXPECT_TRUE(low.genericAxesPresent & GENERIC_AXIS_Y);
}

TEST(HidParser, SignedAxesScaleAroundTheCentre)
{
    Parsed parsed(kGamepad);
    auto data = fill(parsed.info, gamepad_report(0, 8, 0x80, 0x80, -127, 127));
    EXPECT_EQ(data.genericAxisZ, 0);
    EXPECT_EQ(data.genericAxisRz, UINT16_MAX);
    auto centre = fill(parsed.info, gamepad_report(0, 8, 0x80, 0x80, 0, 0));
    EXPECT_NEAR(centre.genericAxisZ, UINT16_MAX / 2, 1);
    // -128 is outside the logical range, so it clamps
    auto clamped = fill(parsed.info, gamepad_report(0, 8, 0x80, 0x80, -128, 0));
    EXPECT_EQ(clamped.genericAxisZ, 0);
}

TEST(HidParser, ReportsWithAnotherIdLeaveStateAlone)
{
    Parsed parsed(kGamepad);
    auto data = fill(parsed.info, gamepad_report(0x0003, 0, 0x10, 0x20, 0, 0));
    Bytes other = gamepad_report(0, 4, 0xFF, 0xFF, 0, 0);
    other[0] = 0x02;
    auto after = fill(parsed.info, other, data);
    EXPECT_EQ(after.genericButtons, 0x0003);
    EXPECT_TRUE(after.dpadUp);
    EXPECT_EQ(after.genericAxisX, data.genericAxisX);
}

TEST(HidParser, NullInfoIsIgnored)
{
    USB_Host_Data_t data = {};
    data.genericButtons = 0x55;
    Bytes report = gamepad_report(0xFFF, 0, 0, 0, 0, 0);
    fill_generic_report(nullptr, report.data(), &data);
    EXPECT_EQ(data.genericButtons, 0x55);
}

TEST(HidParser, PoolsAreReturnedOnFree)
{
    // the item pool is shared and fixed size, so parsing many times only works if frees give it back
    for (int i = 0; i < 50; i++)
    {
        Parsed parsed(kGamepad);
        ASSERT_EQ(parsed.result, HID_PARSE_Successful) << "iteration " << i;
        ASSERT_EQ(parsed.info->TotalReportItems, 17) << "iteration " << i;
    }
}

TEST(HidParser, TruncatedDescriptorsDontCrash)
{
    for (size_t len = 0; len <= kGamepad.size(); len++)
    {
        Bytes desc(kGamepad.begin(), kGamepad.begin() + len);
        Parsed parsed(desc);
    }
}
