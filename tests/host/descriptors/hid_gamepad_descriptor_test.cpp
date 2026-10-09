#include <gtest/gtest.h>
#include <cstring>
#include "firmware_descriptors.hpp"
#include "protocols/hid.hpp"
#include "protocols/xinput.hpp"
#include "struct_fields.hpp"

// The Santroller HID gamepad (HIDGamepadDevice, ModeHid, src/emulation/usb/hid_gamepad_device.cpp) and its
// bluetooth twin (BTGamepadDevice, bt_descriptors.cpp / bt_gamepad.cpp).
//
// Pairing, from the code that sends the reports:
//  - process() fills epin_buf as PCGamepadDpad_Data_t (rid, rsize, then an XInput style report, which is
//    what the mappings' update_hid -> update_xinput write), converts the dpad bits to a hat unless the
//    subtype is a dance pad, and sends sizeof(XInputGamepad_Data_t) bytes, report id included.
//  - Dance pads get the "buttons" descriptor, everything else the "hat" one (report_descriptor()), each
//    with a battery report when a battery is present (m_battery).
//  - The capabilities report (0x10) is sent as 3 bytes: id, subtype, capabilities. The battery report
//    (0x20) as 2 bytes: id, level.
//  - Output report 1 is read by santroller_handle_output_command / the rumble fallback; 0x10 output is
//    the capabilities request.

using namespace hid_desc;

namespace
{
const uint8_t kGamepad = ReportIdGamepad;
const uint8_t kCapabilities = ReportIdSantrollerCapabilities;
const uint8_t kBattery = ReportIdBattery;

// Kernel button usages the descriptor gives each XInput button (hid_reports.h BTN_*)
void expect_common_layout(const Descriptor &d)
{
    // byte 1 (after the id) is the struct's rsize byte, which the descriptor declares as padding
    const Element *rsize = d.element_at(ReportType::Input, kGamepad, 0);
    ASSERT_NE(rsize, nullptr);
    EXPECT_TRUE(rsize->constant());
    EXPECT_EQ(rsize->bit_size, 8u);
    EXPECT_EQ(FIELD(PCGamepadDpad_Data_t, rsize).bit, 8u);

    expect_field(d, ReportType::Input, kGamepad, button(BTN_START), FIELD(PCGamepadDpad_Data_t, start), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_SELECT), FIELD(PCGamepadDpad_Data_t, back), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_THUMBL), FIELD(PCGamepadDpad_Data_t, leftThumbClick), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_THUMBR), FIELD(PCGamepadDpad_Data_t, rightThumbClick), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_TL), FIELD(PCGamepadDpad_Data_t, leftShoulder), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_TR), FIELD(PCGamepadDpad_Data_t, rightShoulder), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_GUIDE), FIELD(PCGamepadDpad_Data_t, guide), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_C), FIELD(PCGamepadDpad_Data_t, capture), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_A), FIELD(PCGamepadDpad_Data_t, a), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_B), FIELD(PCGamepadDpad_Data_t, b), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_X), FIELD(PCGamepadDpad_Data_t, x), true);
    expect_field(d, ReportType::Input, kGamepad, button(BTN_Y), FIELD(PCGamepadDpad_Data_t, y), true);

    // triggers are unsigned bytes, sticks signed 16 bit
    expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopZ), FIELD(PCGamepadDpad_Data_t, leftTrigger), true);
    expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopRz), FIELD(PCGamepadDpad_Data_t, rightTrigger), true);
    expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopX), FIELD(PCGamepadDpad_Data_t, leftStickX), true);
    expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopY), FIELD(PCGamepadDpad_Data_t, leftStickY), true);
    expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopRx), FIELD(PCGamepadDpad_Data_t, rightStickX), true);
    expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopRy), FIELD(PCGamepadDpad_Data_t, rightStickY), true);

    // the six vendor bytes at the end carry the instruments' extra data
    const Element *v20 = d.find(ReportType::Input, kGamepad, vendor(0x20));
    ASSERT_NE(v20, nullptr);
    expect_bytes(d, ReportType::Input, kGamepad, v20->bit_offset, ARRAY_FIELD(PCGamepadDpad_Data_t, reserved_1), true);
    EXPECT_NE(d.find(ReportType::Input, kGamepad, vendor(0x25)), nullptr);
}

struct Variant
{
    const char *name;
    std::vector<uint8_t> bytes;
};
const std::vector<Variant> kHatVariants = {{"hat", bytes_of(kHidGamepadHat)}, {"hat + battery", bytes_of(kHidGamepadHatBattery)}};
const std::vector<Variant> kButtonVariants = {{"buttons", bytes_of(kHidGamepadButtons)},
                                              {"buttons + battery", bytes_of(kHidGamepadButtonsBattery)}};

void expect_report_sizes(const Descriptor &d)
{
    // process() sends sizeof(XInputGamepad_Data_t) bytes, id included, from a PCGamepadDpad_Data_t
    EXPECT_EQ(d.wire_bytes(ReportType::Input, kGamepad), sizeof(XInputGamepad_Data_t));
    EXPECT_EQ(sizeof(PCGamepadDpad_Data_t), sizeof(XInputGamepad_Data_t));
    // capabilities: id, subtype, capabilities
    EXPECT_EQ(d.wire_bytes(ReportType::Input, kCapabilities), 3u);
    EXPECT_TRUE(d.has_report(ReportType::Output, kCapabilities));
    EXPECT_TRUE(d.has_report(ReportType::Output, kGamepad));
    // ReportIdPs4Feature, which get_report uses to tell a PS3 / PS4 / PS5 apart
    EXPECT_TRUE(d.has_report(ReportType::Feature, ReportIdPs4Feature));
}
} // namespace

TEST(HidGamepadDescriptor, HatLayoutMatchesTheReportStruct)
{
    for (const auto &desc : kHatVariants)
    {
        SCOPED_TRACE(desc.name);
        auto d = parse(desc.bytes);
        ASSERT_TRUE(d.ok) << d.error;
        expect_report_sizes(d);
        expect_common_layout(d);
        // the hat goes where the dpad bits were (dpad_bindings turns them into 0 - 7, 8 for neutral)
        expect_field(d, ReportType::Input, kGamepad, desktop(kDesktopHat), FIELD(PCGamepadDpad_Data_t, dpad), true);
        EXPECT_EQ(d.find(ReportType::Input, kGamepad, desktop(kDesktopHat))->logical_max, 7);
        EXPECT_EQ(d.find(ReportType::Input, kGamepad, desktop(kDesktopHat))->logical_min, 0);
        EXPECT_EQ(d.find(ReportType::Input, kGamepad, button(BTN_UP)), nullptr);
    }
}

TEST(HidGamepadDescriptor, DancePadLayoutHasDpadButtons)
{
    for (const auto &desc : kButtonVariants)
    {
        SCOPED_TRACE(desc.name);
        auto d = parse(desc.bytes);
        ASSERT_TRUE(d.ok) << d.error;
        expect_report_sizes(d);
        expect_common_layout(d);
        // dance pads keep the XInput dpad bits as four buttons
        expect_field(d, ReportType::Input, kGamepad, button(BTN_UP), FIELD(XInputGamepad_Data_t, dpadUp), true);
        expect_field(d, ReportType::Input, kGamepad, button(BTN_DOWN), FIELD(XInputGamepad_Data_t, dpadDown), true);
        expect_field(d, ReportType::Input, kGamepad, button(BTN_LEFT), FIELD(XInputGamepad_Data_t, dpadLeft), true);
        expect_field(d, ReportType::Input, kGamepad, button(BTN_RIGHT), FIELD(XInputGamepad_Data_t, dpadRight), true);
        EXPECT_EQ(d.find(ReportType::Input, kGamepad, desktop(kDesktopHat)), nullptr);
    }
}

TEST(HidGamepadDescriptor, PcAndXInputStructsShareTheLayout)
{
    // process() writes through both views of the same buffer
    EXPECT_EQ(FIELD(PCGamepadDpad_Data_t, start).bit, FIELD(XInputGamepad_Data_t, start).bit);
    EXPECT_EQ(FIELD(PCGamepadDpad_Data_t, dpad).bit, FIELD(XInputGamepad_Data_t, dpadUp).bit);
    EXPECT_EQ(FIELD(PCGamepadDpad_Data_t, y).bit, FIELD(XInputGamepad_Data_t, y).bit);
    EXPECT_EQ(offsetof(PCGamepadDpad_Data_t, leftStickX), offsetof(XInputGamepad_Data_t, leftStickX));
    EXPECT_EQ(offsetof(PCGamepadDpad_Data_t, reserved_1), offsetof(XInputGamepad_Data_t, reserved_1));
}

// The instrument subtypes write their XInput report into the same buffer (initialize() and the
// mappings' update_hid), and only sizeof(XInputGamepad_Data_t) bytes go out
TEST(HidGamepadDescriptor, InstrumentReportsFitTheGamepadReport)
{
    const size_t report = parse(kHidGamepadHat).wire_bytes(ReportType::Input, kGamepad);
    EXPECT_EQ(sizeof(XInputRockBandDrums_Data_t), report);
    EXPECT_EQ(sizeof(XInputGuitarHeroDrums_Data_t), report);
    EXPECT_EQ(sizeof(XInputGuitarHeroGuitar_Data_t), report);
    EXPECT_EQ(sizeof(XInputRockBandGuitar_Data_t), report);
    EXPECT_EQ(sizeof(XInputRockBandProGuitar_Data_t), report);
    EXPECT_EQ(sizeof(XInputRockBandKeyboard_Data_t), report);
    EXPECT_EQ(sizeof(XInputGHLGuitar_Data_t), report);
    EXPECT_EQ(sizeof(XInputDJHTurntable_Data_t), report);
}

TEST(HidGamepadDescriptor, BatteryReportOnlyWithABattery)
{
    for (const auto &desc : {kHatVariants[1], kButtonVariants[1]})
    {
        SCOPED_TRACE(desc.name);
        auto d = parse(desc.bytes);
        ASSERT_TRUE(d.ok) << d.error;
        // id, level
        EXPECT_EQ(d.wire_bytes(ReportType::Input, kBattery), 2u);
        // Battery Strength (Generic Device Controls 0x20), 0 - 100 percent
        const Element *level = d.find(ReportType::Input, kBattery, usage(0x06, 0x20));
        ASSERT_NE(level, nullptr);
        EXPECT_EQ(level->bit_offset, 0u);
        EXPECT_EQ(level->bit_size, 8u);
        EXPECT_EQ(level->logical_min, 0);
        EXPECT_EQ(level->logical_max, 100);
    }
    for (const auto &desc : {kHatVariants[0], kButtonVariants[0]})
        EXPECT_FALSE(parse(desc.bytes).has_report(ReportType::Input, kBattery)) << desc.name;
}

TEST(HidGamepadDescriptor, OutputReportHoldsTheRumbleAndCommandReports)
{
    auto d = parse(kHidGamepadHat);
    ASSERT_TRUE(d.ok) << d.error;
    // set_report reads up to buffer[4] for rumble and santroller_handle_output_command needs id, command
    // and up to three arguments (rgb), all within the declared 8 bytes
    EXPECT_GE(d.wire_bytes(ReportType::Output, kGamepad), 5u);
    EXPECT_GE(d.report_bytes(ReportType::Output, kGamepad), 4u);
}

// santroller_handle_output_command (src/emulation/santroller_commands.cpp) passes the feedback command
// (0x5B) to Instance::process_feedback_report, which ignores it unless at least 9 bytes follow the report
// id (src/instance.cpp:136) and reads a 10th for note hits (src/instance.cpp:160). Hosts that size output
// reports from the descriptor (Windows HID / hidapi) can only send it if output report 1 is that long.
TEST(HidGamepadDescriptor, OutputReportHoldsTheFeedbackCommand)
{
    for (const auto &desc : {kHatVariants[0], kHatVariants[1], kButtonVariants[0], kButtonVariants[1]})
    {
        auto d = parse(desc.bytes);
        ASSERT_TRUE(d.ok) << d.error;
        EXPECT_EQ(d.report_bytes(ReportType::Output, kGamepad), 10u) << desc.name;
    }
}

// Bluetooth advertises the same two layouts (no battery report) and sends the same buffer, minus the id
TEST(BtGamepadDescriptor, SameLayoutAsUsb)
{
    EXPECT_EQ(std::vector<uint8_t>(desc_hid_report_hat, desc_hid_report_hat + sizeof(desc_hid_report_hat)),
              bytes_of(kHidGamepadHat));
    EXPECT_EQ(std::vector<uint8_t>(desc_hid_report_buttons, desc_hid_report_buttons + sizeof(desc_hid_report_buttons)),
              bytes_of(kHidGamepadButtons));
}
