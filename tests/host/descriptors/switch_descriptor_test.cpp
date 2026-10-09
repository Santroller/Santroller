#include <gtest/gtest.h>
#include "firmware_descriptors.hpp"
#include "protocols/switch.hpp"
#include "protocols/switch_arcade.hpp"
#include "struct_fields.hpp"

// Switch emulation (ModeSwitch, src/emulation/usb/switch_device.cpp).
//
// Pairing, from the code that sends the reports:
//  - SwitchArcadeDevice (Project Diva and Taiko subtypes, instance_factory.cpp) sends
//    sizeof(SwitchArcadeReport), no report id, and ignores output reports.
//  - SwitchGamepadDevice (everything else) sends the 64 byte SwitchProGamepad_Data_t (report 0x30, id in
//    the struct) and 64 byte replies from its report buffer (report[0] = 0x21 / 0x81), and reads output
//    reports with the id in buffer[0].

using namespace hid_desc;

// ---------------------------------------------------------------------------------------------
// HORI style arcade controllers

TEST(SwitchArcadeDescriptor, ReportSizes)
{
    auto d = parse(kSwitchArcade);
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_FALSE(d.uses_report_ids);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, 0), sizeof(SwitchArcadeReport));
    EXPECT_EQ(d.wire_bytes(ReportType::Output, 0), 8u);
}

TEST(SwitchArcadeDescriptor, ButtonsMatchTheButtonMasks)
{
    auto d = parse(kSwitchArcade);
    ASSERT_TRUE(d.ok) << d.error;
    // switch_arcade_set_button puts mask bits 0 - 7 in buttons_low and 8 - 15 in buttons_high
    const uint32_t low = FIELD(SwitchArcadeReport, buttons_low).bit;
    const uint32_t high = FIELD(SwitchArcadeReport, buttons_high).bit;
    EXPECT_EQ(high, low + 8);
    const uint16_t masks[] = {SwitchArcade_Y, SwitchArcade_B, SwitchArcade_A, SwitchArcade_X, SwitchArcade_L,
                              SwitchArcade_R, SwitchArcade_ZL, SwitchArcade_ZR, SwitchArcade_Minus, SwitchArcade_Plus,
                              SwitchArcade_LS, SwitchArcade_RS, SwitchArcade_Home, SwitchArcade_Capture};
    // HORI order: button n is mask bit n - 1
    for (uint16_t n = 1; n <= 14; n++)
    {
        const Element *e = d.find(ReportType::Input, 0, button(n));
        ASSERT_NE(e, nullptr) << n;
        EXPECT_EQ(e->bit_size, 1u);
        EXPECT_EQ(uint32_t(1) << (e->bit_offset - low), masks[n - 1]) << "button " << n;
    }
}

TEST(SwitchArcadeDescriptor, HatAndSticksMatchTheStruct)
{
    auto d = parse(kSwitchArcade);
    ASSERT_TRUE(d.ok) << d.error;
    // a 4 bit hat (0 - 7, null outside) in the low nibble of the hat byte, padding above it
    const FieldBits hat_byte = FIELD(SwitchArcadeReport, hat);
    const Element *hat = d.find(ReportType::Input, 0, desktop(kDesktopHat));
    ASSERT_NE(hat, nullptr);
    EXPECT_EQ(hat->bit_offset, hat_byte.bit);
    EXPECT_EQ(hat->bit_size, 4u);
    EXPECT_EQ(hat->logical_max, 7);
    EXPECT_TRUE(hat->null_state());
    const Element *pad = d.element_at(ReportType::Input, 0, hat_byte.bit + 4);
    ASSERT_NE(pad, nullptr);
    EXPECT_TRUE(pad->constant());
    // switch_arcade_finish_hat only produces 0 - 8 (8 is neutral), which fit the low nibble
    for (uint8_t directions = 0; directions < 16; directions++)
    {
        SwitchArcadeReport report = {0, 0, uint8_t(directions << 4), 128, 128, 128, 128, 0};
        switch_arcade_finish_hat(report);
        EXPECT_LE(report.hat, 8);
    }

    expect_field(d, ReportType::Input, 0, desktop(kDesktopX), FIELD(SwitchArcadeReport, lx), false);
    expect_field(d, ReportType::Input, 0, desktop(kDesktopY), FIELD(SwitchArcadeReport, ly), false);
    expect_field(d, ReportType::Input, 0, desktop(kDesktopZ), FIELD(SwitchArcadeReport, rx), false);
    expect_field(d, ReportType::Input, 0, desktop(kDesktopRz), FIELD(SwitchArcadeReport, ry), false);
    expect_field(d, ReportType::Input, 0, vendor(0x20), FIELD(SwitchArcadeReport, vendor), false);
    EXPECT_EQ(d.find(ReportType::Input, 0, desktop(kDesktopX))->logical_max, 255);
}

// ---------------------------------------------------------------------------------------------
// Pro Controller. Consoles may care about the exact bytes, so the descriptor is the one a real Pro
// Controller (HAC-013) reports, byte for byte. These 203 bytes are switchpro_hid_report_descriptor.txt from
// https://github.com/DJm00n/ControllersInfo/tree/master/switchpro (dumped from the controller); GP2040-CE's
// SwitchProDescriptors.h has the same bytes.
namespace
{
const uint8_t kRealProController[] = {
    0x05, 0x01, 0x15, 0x00, 0x09, 0x04, 0xA1, 0x01, 0x85, 0x30, 0x05, 0x01, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x0A, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0A, 0x55, 0x00, 0x65, 0x00, 0x81, 0x02,
    0x05, 0x09, 0x19, 0x0B, 0x29, 0x0E, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x04, 0x81, 0x02,
    0x75, 0x01, 0x95, 0x02, 0x81, 0x03, 0x0B, 0x01, 0x00, 0x01, 0x00, 0xA1, 0x00, 0x0B, 0x30, 0x00,
    0x01, 0x00, 0x0B, 0x31, 0x00, 0x01, 0x00, 0x0B, 0x32, 0x00, 0x01, 0x00, 0x0B, 0x35, 0x00, 0x01,
    0x00, 0x15, 0x00, 0x27, 0xFF, 0xFF, 0x00, 0x00, 0x75, 0x10, 0x95, 0x04, 0x81, 0x02, 0xC0, 0x0B,
    0x39, 0x00, 0x01, 0x00, 0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3B, 0x01, 0x65, 0x14, 0x75,
    0x04, 0x95, 0x01, 0x81, 0x02, 0x05, 0x09, 0x19, 0x0F, 0x29, 0x12, 0x15, 0x00, 0x25, 0x01, 0x75,
    0x01, 0x95, 0x04, 0x81, 0x02, 0x75, 0x08, 0x95, 0x34, 0x81, 0x03, 0x06, 0x00, 0xFF, 0x85, 0x21,
    0x09, 0x01, 0x75, 0x08, 0x95, 0x3F, 0x81, 0x03, 0x85, 0x81, 0x09, 0x02, 0x75, 0x08, 0x95, 0x3F,
    0x81, 0x03, 0x85, 0x01, 0x09, 0x03, 0x75, 0x08, 0x95, 0x3F, 0x91, 0x83, 0x85, 0x10, 0x09, 0x04,
    0x75, 0x08, 0x95, 0x3F, 0x91, 0x83, 0x85, 0x80, 0x09, 0x05, 0x75, 0x08, 0x95, 0x3F, 0x91, 0x83,
    0x85, 0x82, 0x09, 0x06, 0x75, 0x08, 0x95, 0x3F, 0x91, 0x83, 0xC0};
static_assert(sizeof(kRealProController) == 203, "the real descriptor is 203 bytes");
} // namespace

TEST(SwitchProDescriptor, IsTheRealProControllerDescriptor)
{
    ASSERT_EQ(sizeof(kSwitchPro), sizeof(kRealProController));
    for (size_t i = 0; i < sizeof(kRealProController); i++)
        EXPECT_EQ(kSwitchPro[i], kRealProController[i]) << "byte " << i;
}

// Like the real one, the descriptor's report 0x30 fields don't describe the standard full mode report
// the controller actually sends (12 bit sticks etc.); consoles and drivers (hid-nintendo, SDL) parse the
// raw bytes. So beyond the bytes above only report ids and lengths are checked.

TEST(SwitchProDescriptor, ReportIds)
{
    auto d = parse(kSwitchPro);
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_TRUE(d.has_report(ReportType::Input, ReportSwitchOutput30));
    EXPECT_TRUE(d.has_report(ReportType::Input, ReportSwitchOutput21));
    EXPECT_TRUE(d.has_report(ReportType::Input, ReportSwitchInput));
    EXPECT_TRUE(d.has_report(ReportType::Output, ReportSwitchFeature));
    EXPECT_TRUE(d.has_report(ReportType::Output, ReportSwitchOutput10));
    EXPECT_TRUE(d.has_report(ReportType::Output, ReportSwitchConfiguration));
    EXPECT_TRUE(d.has_report(ReportType::Output, ReportSwitchOutput82));
}

TEST(SwitchProDescriptor, ReplyAndCommandReportsAreSixtyFourBytes)
{
    auto d = parse(kSwitchPro);
    ASSERT_TRUE(d.ok) << d.error;
    // subcommand replies (0x21) and USB replies (0x81) go out as the 64 byte report buffer
    EXPECT_EQ(d.wire_bytes(ReportType::Input, ReportSwitchOutput21), size_t(CFG_TUD_HID_EP_BUFSIZE));
    EXPECT_EQ(d.wire_bytes(ReportType::Input, ReportSwitchInput), size_t(CFG_TUD_HID_EP_BUFSIZE));
    // commands come in on the 64 byte OUT endpoint
    for (uint8_t id : {uint8_t(ReportSwitchFeature), uint8_t(ReportSwitchOutput10), uint8_t(ReportSwitchConfiguration),
                       uint8_t(ReportSwitchOutput82)})
        EXPECT_EQ(d.wire_bytes(ReportType::Output, id), size_t(CFG_TUD_HID_EP_BUFSIZE)) << int(id);
    EXPECT_EQ(sizeof(SwitchProGamepad_Data_t), size_t(CFG_TUD_HID_EP_BUFSIZE));
}

// The input report the firmware sends most (0x30, sizeof(SwitchProGamepad_Data_t) = 64 bytes from
// SwitchGamepadDevice::process) ends with 52 bytes of Input (Constant) padding (75 08 95 34 81 03), as in
// the descriptor dumped from a real Pro Controller (HAC-013, DJm00n/ControllersInfo), making it a 63 byte
// input report like 0x21 and 0x81.
TEST(SwitchProDescriptor, FullReportLengthMatchesTheStruct)
{
    auto d = parse(kSwitchPro);
    ASSERT_TRUE(d.ok) << d.error;
    EXPECT_EQ(d.wire_bytes(ReportType::Input, ReportSwitchOutput30), sizeof(SwitchProGamepad_Data_t));
    EXPECT_FALSE(d.has_report(ReportType::Output, ReportSwitchOutput30));
}
