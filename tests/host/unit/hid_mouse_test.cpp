#include <gtest/gtest.h>
#include <vector>
#include "protocols/hid_mouse.hpp"

namespace
{
using Bytes = std::vector<uint8_t>;
constexpr uint16_t kCentre = UINT16_MAX / 2;

// 3 buttons + padding, then X / Y / wheel as relative 8 bit values from -127 to 127, then AC pan
const Bytes kMouse = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x05, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
    0x05, 0x0C, 0x0A, 0x38, 0x02, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x01, 0x81, 0x06,
    0xC0, 0xC0};

// Report id 2, 16 bit X / Y from -32767 to 32767 (a gaming mouse)
const Bytes kMouse16 = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x02, 0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
    0x95, 0x01, 0x75, 0x05, 0x81, 0x01,
    0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x16, 0x01, 0x80, 0x26, 0xFF, 0x7F, 0x75, 0x10, 0x95, 0x02, 0x81, 0x06,
    0xC0, 0xC0};

struct Mouse
{
    HID_ReportInfo_t *info = nullptr;
    HidMouseDecoder decoder;
    explicit Mouse(const Bytes &desc)
    {
        uint8_t result = USB_ProcessHIDReport(desc.data(), desc.size(), &info);
        EXPECT_EQ(result, HID_PARSE_Successful);
        decoder.set_report_info(info);
    }
    ~Mouse() { USB_FreeReportInfo(info); }
    void report(const Bytes &data, uint32_t now_us = 1000) { decoder.handle_report(data.data(), now_us); }
};
} // namespace

TEST(HidMouse, ButtonsPressAndRelease)
{
    Mouse mouse(kMouse);
    mouse.report({0x05, 0, 0, 0, 0});
    EXPECT_TRUE(mouse.decoder.button(HidMouseDecoder::Left));
    EXPECT_FALSE(mouse.decoder.button(HidMouseDecoder::Right));
    EXPECT_TRUE(mouse.decoder.button(HidMouseDecoder::Middle));
    mouse.report({0x02, 0, 0, 0, 0});
    EXPECT_FALSE(mouse.decoder.button(HidMouseDecoder::Left));
    EXPECT_TRUE(mouse.decoder.button(HidMouseDecoder::Right));
    EXPECT_FALSE(mouse.decoder.button(HidMouseDecoder::Middle));
}

TEST(HidMouse, PositiveMovementMovesAwayFromCentre)
{
    Mouse mouse(kMouse);
    mouse.report({0x00, 10, 1, 0, 0});
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000), kCentre + 10 * MOUSE_MOVE_SCALE);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveY, 1000), kCentre + MOUSE_MOVE_SCALE);
}

TEST(HidMouse, NegativeMovementMovesBelowCentre)
{
    Mouse mouse(kMouse);
    // -10 and -1 as 8 bit two's complement
    mouse.report({0x00, 0xF6, 0xFF, 0, 0});
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000), kCentre - 10 * MOUSE_MOVE_SCALE);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveY, 1000), kCentre - MOUSE_MOVE_SCALE);
}

TEST(HidMouse, SixteenBitNegativeMovement)
{
    Mouse mouse(kMouse16);
    // report id, buttons, X = -2, Y = 3
    mouse.report({0x02, 0x00, 0xFE, 0xFF, 0x03, 0x00});
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000), kCentre - 2 * MOUSE_MOVE_SCALE);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveY, 1000), kCentre + 3 * MOUSE_MOVE_SCALE);
}

TEST(HidMouse, ScrollUsesTheScrollScale)
{
    Mouse mouse(kMouse);
    mouse.report({0x00, 0, 0, 1, 0xFF}); // wheel up one notch, pan left one notch
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::ScrollY, 1000), kCentre + MOUSE_SCROLL_SCALE);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::ScrollX, 1000), kCentre - MOUSE_SCROLL_SCALE);
}

TEST(HidMouse, FullEightBitRangeFitsTheAxis)
{
    Mouse mouse(kMouse);
    mouse.report({0x00, 127, 0x81, 0, 0}); // +127 and -127
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000), kCentre + 127 * MOUSE_MOVE_SCALE);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveY, 1000), kCentre - 127 * MOUSE_MOVE_SCALE);
}

TEST(HidMouse, LargeMovementClamps)
{
    Mouse mouse(kMouse16);
    mouse.report({0x02, 0x00, 0xC8, 0x00, 0x38, 0xFF}); // +200 and -200
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000), UINT16_MAX);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveY, 1000), 0);
}

TEST(HidMouse, MovementStopsOnceReportsDo)
{
    Mouse mouse(kMouse);
    mouse.report({0x00, 10, 0, 0, 0}, 1000);
    EXPECT_NE(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000 + MOUSE_IDLE_US), kCentre);
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000 + MOUSE_IDLE_US + 1), kCentre);
}

TEST(HidMouse, IdleTimeoutSurvivesTimerWrap)
{
    Mouse mouse(kMouse);
    mouse.report({0x00, 10, 0, 0, 0}, UINT32_MAX - 100);
    EXPECT_NE(mouse.decoder.axis(HidMouseDecoder::MoveX, 200), kCentre);
}

TEST(HidMouse, ReportsForAnotherIdAreIgnored)
{
    Mouse mouse(kMouse16);
    mouse.report({0x02, 0x01, 0x05, 0x00, 0x00, 0x00});
    mouse.report({0x03, 0x00, 0x50, 0x00, 0x00, 0x00}, 2000);
    EXPECT_TRUE(mouse.decoder.button(HidMouseDecoder::Left));
    EXPECT_EQ(mouse.decoder.axis(HidMouseDecoder::MoveX, 1000), kCentre + 5 * MOUSE_MOVE_SCALE);
}

TEST(HidMouse, NoReportInfoStaysAtRest)
{
    HidMouseDecoder decoder;
    decoder.set_report_info(nullptr);
    const Bytes report = {0x07, 10, 10, 1, 1};
    decoder.handle_report(report.data(), 1000);
    EXPECT_FALSE(decoder.button(HidMouseDecoder::Left));
    EXPECT_EQ(decoder.axis(HidMouseDecoder::MoveX, 1000), kCentre);
    EXPECT_EQ(decoder.axis(HidMouseDecoder::AxisCount, 1000), kCentre);
}
