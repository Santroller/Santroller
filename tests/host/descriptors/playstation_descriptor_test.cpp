#include <gtest/gtest.h>
#include <cstring>
#include "firmware_descriptors.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/ps5.hpp"
#include "struct_fields.hpp"

// PS3 / PS4 / PS5 emulation (ModePs3 / ModeWiiRb, ModePs4, ModePs5).
//
// Pairing, from the code that sends the reports:
//  - PS3GamepadDevice (src/emulation/usb/ps3_device.cpp): Gamepad and Taiko subtypes
//    (uses_ps3_gamepad_report) advertise the first party DualShock 3 descriptor and send
//    sizeof(PS3Gamepad_Data_t), report id included; every other subtype advertises the third party
//    descriptor (no report ids) and sends sizeof(PS3Dpad_Data_t) from a PS3Dpad_Data_t, or one of the
//    instrument structs written over the same buffer. Output reports are read as PS3InstrumentOutput.
//  - PS4GamepadDevice (ps4_device.cpp) sends sizeof(PS4Dpad_Data_t), id included, reads output report 5
//    as ps4_output_report, and answers the feature / auth reports with ps4_feature_config (48 bytes,
//    ps4_device.cpp:30-36), AuthPageSizeReport, and the auth device's AuthReport / AuthStatusReport.
//  - PS5GamepadDevice (ps5_device.cpp) the same with PS5Dpad_Data_t, ps5_output_report (report 2) and
//    ps5_feature_config (48 bytes, ps5_device.cpp:36-42).

using namespace hid_desc;

namespace
{
const uint8_t kGamepad = ReportIdGamepad;

Descriptor parsed(const std::vector<uint8_t> &bytes)
{
    auto d = parse(bytes);
    EXPECT_TRUE(d.ok) << d.error;
    return d;
}

// One data byte of the descriptor (whatever its usage) sits on this struct field
void expect_byte_element(const Descriptor &d, ReportType type, uint8_t id, const FieldBits &field, bool struct_has_report_id)
{
    SCOPED_TRACE(field.name);
    ASSERT_EQ(field.width, 8u);
    ASSERT_GE(field.bit, struct_has_report_id ? 8u : 0u);
    const uint32_t bit = field.bit - (struct_has_report_id ? 8 : 0);
    const Element *e = d.element_at(type, id, bit);
    ASSERT_NE(e, nullptr);
    EXPECT_FALSE(e->constant());
    EXPECT_EQ(e->bit_offset, bit);
    EXPECT_EQ(e->bit_size, 8u);
}
} // namespace

// ---------------------------------------------------------------------------------------------
// PS3, third party (instruments and anything that isn't a DualShock 3)

TEST(Ps3ThirdPartyDescriptor, ReportSize)
{
    auto d = parsed(bytes_of(kPs3ThirdParty));
    EXPECT_FALSE(d.uses_report_ids);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, 0), sizeof(PS3Dpad_Data_t));
    // set_report reads output reports as PS3InstrumentOutput (player LEDs, instrument commands)
    EXPECT_EQ(d.wire_bytes(ReportType::Output, 0), sizeof(PS3InstrumentOutput));
}

TEST(Ps3ThirdPartyDescriptor, FieldsMatchTheReportStruct)
{
    auto d = parsed(bytes_of(kPs3ThirdParty));
    const ReportType in = ReportType::Input;
    // buttons 1 - 13: square cross circle triangle L1 R1 L2 R2 select start L3 R3 PS
    expect_field(d, in, 0, button(1), FIELD(PS3Dpad_Data_t, x), false);
    expect_field(d, in, 0, button(2), FIELD(PS3Dpad_Data_t, a), false);
    expect_field(d, in, 0, button(3), FIELD(PS3Dpad_Data_t, b), false);
    expect_field(d, in, 0, button(4), FIELD(PS3Dpad_Data_t, y), false);
    expect_field(d, in, 0, button(5), FIELD(PS3Dpad_Data_t, leftShoulder), false);
    expect_field(d, in, 0, button(6), FIELD(PS3Dpad_Data_t, rightShoulder), false);
    expect_field(d, in, 0, button(7), FIELD(PS3Dpad_Data_t, l2), false);
    expect_field(d, in, 0, button(8), FIELD(PS3Dpad_Data_t, r2), false);
    expect_field(d, in, 0, button(9), FIELD(PS3Dpad_Data_t, back), false);
    expect_field(d, in, 0, button(10), FIELD(PS3Dpad_Data_t, start), false);
    expect_field(d, in, 0, button(11), FIELD(PS3Dpad_Data_t, leftThumbClick), false);
    expect_field(d, in, 0, button(12), FIELD(PS3Dpad_Data_t, rightThumbClick), false);
    expect_field(d, in, 0, button(13), FIELD(PS3Dpad_Data_t, guide), false);
    expect_field(d, in, 0, desktop(kDesktopHat), FIELD(PS3Dpad_Data_t, dpad), false);
    expect_field(d, in, 0, desktop(kDesktopX), FIELD(PS3Dpad_Data_t, leftStickX), false);
    expect_field(d, in, 0, desktop(kDesktopY), FIELD(PS3Dpad_Data_t, leftStickY), false);
    expect_field(d, in, 0, desktop(kDesktopZ), FIELD(PS3Dpad_Data_t, rightStickX), false);
    expect_field(d, in, 0, desktop(kDesktopRz), FIELD(PS3Dpad_Data_t, rightStickY), false);
    // the twelve pressure bytes are vendor usages 0x20 - 0x2B, in struct order
    expect_field(d, in, 0, vendor(0x20), FIELD(PS3Dpad_Data_t, pressureDpadUp), false);
    expect_field(d, in, 0, vendor(0x21), FIELD(PS3Dpad_Data_t, pressureDpadRight), false);
    expect_field(d, in, 0, vendor(0x22), FIELD(PS3Dpad_Data_t, pressureDpadLeft), false);
    expect_field(d, in, 0, vendor(0x23), FIELD(PS3Dpad_Data_t, pressureDpadDown), false);
    expect_field(d, in, 0, vendor(0x24), FIELD(PS3Dpad_Data_t, leftTrigger), false);
    expect_field(d, in, 0, vendor(0x25), FIELD(PS3Dpad_Data_t, rightTrigger), false);
    expect_field(d, in, 0, vendor(0x26), FIELD(PS3Dpad_Data_t, pressureL1), false);
    expect_field(d, in, 0, vendor(0x27), FIELD(PS3Dpad_Data_t, pressureR1), false);
    expect_field(d, in, 0, vendor(0x28), FIELD(PS3Dpad_Data_t, pressureTriangle), false);
    expect_field(d, in, 0, vendor(0x29), FIELD(PS3Dpad_Data_t, pressureCircle), false);
    expect_field(d, in, 0, vendor(0x2A), FIELD(PS3Dpad_Data_t, pressureCross), false);
    expect_field(d, in, 0, vendor(0x2B), FIELD(PS3Dpad_Data_t, pressureSquare), false);
    // and the 10 bit motion axes, as 16 bit vendor usages 0x2C - 0x2F
    expect_field(d, in, 0, vendor(0x2C), FIELD(PS3Dpad_Data_t, accelX), false);
    expect_field(d, in, 0, vendor(0x2D), FIELD(PS3Dpad_Data_t, accelZ), false);
    expect_field(d, in, 0, vendor(0x2E), FIELD(PS3Dpad_Data_t, accelY), false);
    expect_field(d, in, 0, vendor(0x2F), FIELD(PS3Dpad_Data_t, gyro), false);
    EXPECT_EQ(d.find(in, 0, vendor(0x2C))->logical_max, 0x3FF);
    EXPECT_EQ(d.find(in, 0, desktop(kDesktopHat))->logical_max, 7);
}

TEST(Ps3ThirdPartyDescriptor, InstrumentReportsFitTheReport)
{
    // written over the same buffer as PS3Dpad_Data_t, of which sizeof(PS3Dpad_Data_t) bytes go out
    const size_t report = sizeof(PS3Dpad_Data_t);
    EXPECT_EQ(sizeof(PS3ThirdPartyGamepad_Data_t), report);
    EXPECT_EQ(sizeof(PS3RockBandDrums_Data_t), report);
    EXPECT_EQ(sizeof(PS3GuitarHeroDrums_Data_t), report);
    EXPECT_EQ(sizeof(PS3GuitarHeroGuitar_Data_t), report);
    EXPECT_EQ(sizeof(PS3RockBandGuitar_Data_t), report);
    EXPECT_EQ(sizeof(PS3PowerGigGuitar_Data_t), report);
    EXPECT_EQ(sizeof(PS3PowerGigDrums_Data_t), report);
    EXPECT_EQ(sizeof(PS3RockBandProGuitar_Data_t), report);
    EXPECT_EQ(sizeof(PS3RockBandProKeyboard_Data_t), report);
    EXPECT_EQ(sizeof(PS3DJHTurntable_Data_t), report);
    EXPECT_EQ(sizeof(PS3GHLGuitar_Data_t), report);
}

// ---------------------------------------------------------------------------------------------
// PS3, first party (DualShock 3 layout, Gamepad and Taiko)

TEST(Ps3FirstPartyDescriptor, ReportSize)
{
    auto d = parsed(bytes_of(kPs3FirstParty));
    EXPECT_TRUE(d.uses_report_ids);
    EXPECT_EQ(d.wire_bytes(ReportType::Input, kGamepad), sizeof(PS3Gamepad_Data_t));
    EXPECT_EQ(d.wire_bytes(ReportType::Input, kGamepad), size_t(PS3_REPORT_BUFFER_SIZE + 1));
    // the DS3's 48 byte feature reports
    for (uint8_t id : {uint8_t(ReportIdPs302), uint8_t(ReportIdPs3EE), uint8_t(ReportIdPs3EF)})
        EXPECT_EQ(d.report_bytes(ReportType::Feature, id), size_t(PS3_REPORT_BUFFER_SIZE)) << int(id);
}

TEST(Ps3FirstPartyDescriptor, FieldsMatchTheReportStruct)
{
    auto d = parsed(bytes_of(kPs3FirstParty));
    const ReportType in = ReportType::Input;
    // byte 1 is reserved
    ASSERT_NE(d.element_at(in, kGamepad, 0), nullptr);
    EXPECT_TRUE(d.element_at(in, kGamepad, 0)->constant());
    // buttons 1 - 17: select L3 R3 start up right down left L2 R2 L1 R1 triangle circle cross square PS
    expect_field(d, in, kGamepad, button(1), FIELD(PS3Gamepad_Data_t, back), true);
    expect_field(d, in, kGamepad, button(2), FIELD(PS3Gamepad_Data_t, leftThumbClick), true);
    expect_field(d, in, kGamepad, button(3), FIELD(PS3Gamepad_Data_t, rightThumbClick), true);
    expect_field(d, in, kGamepad, button(4), FIELD(PS3Gamepad_Data_t, start), true);
    expect_field(d, in, kGamepad, button(5), FIELD(PS3Gamepad_Data_t, dpadUp), true);
    expect_field(d, in, kGamepad, button(6), FIELD(PS3Gamepad_Data_t, dpadRight), true);
    expect_field(d, in, kGamepad, button(7), FIELD(PS3Gamepad_Data_t, dpadDown), true);
    expect_field(d, in, kGamepad, button(8), FIELD(PS3Gamepad_Data_t, dpadLeft), true);
    expect_field(d, in, kGamepad, button(9), FIELD(PS3Gamepad_Data_t, l2), true);
    expect_field(d, in, kGamepad, button(10), FIELD(PS3Gamepad_Data_t, r2), true);
    expect_field(d, in, kGamepad, button(11), FIELD(PS3Gamepad_Data_t, leftShoulder), true);
    expect_field(d, in, kGamepad, button(12), FIELD(PS3Gamepad_Data_t, rightShoulder), true);
    expect_field(d, in, kGamepad, button(13), FIELD(PS3Gamepad_Data_t, y), true);
    expect_field(d, in, kGamepad, button(14), FIELD(PS3Gamepad_Data_t, b), true);
    expect_field(d, in, kGamepad, button(15), FIELD(PS3Gamepad_Data_t, a), true);
    expect_field(d, in, kGamepad, button(16), FIELD(PS3Gamepad_Data_t, x), true);
    expect_field(d, in, kGamepad, button(17), FIELD(PS3Gamepad_Data_t, guide), true);
    expect_field(d, in, kGamepad, button(18), FIELD(PS3Gamepad_Data_t, capture), true);
    expect_field(d, in, kGamepad, desktop(kDesktopX), FIELD(PS3Gamepad_Data_t, leftStickX), true);
    expect_field(d, in, kGamepad, desktop(kDesktopY), FIELD(PS3Gamepad_Data_t, leftStickY), true);
    expect_field(d, in, kGamepad, desktop(kDesktopZ), FIELD(PS3Gamepad_Data_t, rightStickX), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRz), FIELD(PS3Gamepad_Data_t, rightStickY), true);
    // 19 pointer usage bytes: four unknown, the twelve pressures, three unknown
    expect_byte_element(d, in, kGamepad, FIELD(PS3Gamepad_Data_t, pressureDpadUp), true);
    expect_byte_element(d, in, kGamepad, FIELD(PS3Gamepad_Data_t, leftTrigger), true);
    expect_byte_element(d, in, kGamepad, FIELD(PS3Gamepad_Data_t, rightTrigger), true);
    expect_byte_element(d, in, kGamepad, FIELD(PS3Gamepad_Data_t, pressureSquare), true);
    // the status bytes are padding to the host
    for (const FieldBits &status : {FIELD(PS3Gamepad_Data_t, charge), FIELD(PS3Gamepad_Data_t, battery_status), FIELD(PS3Gamepad_Data_t, connection)})
    {
        const Element *e = d.element_at(in, kGamepad, status.bit - 8);
        ASSERT_NE(e, nullptr) << status.name;
        EXPECT_TRUE(e->constant()) << status.name;
    }
    // then the four 16 bit motion axes, 0 - 1023 (declared without a usage, unlike the rest)
    for (const FieldBits &axis : {FIELD(PS3Gamepad_Data_t, accelX), FIELD(PS3Gamepad_Data_t, accelY), FIELD(PS3Gamepad_Data_t, accelZ),
                                  FIELD(PS3Gamepad_Data_t, gyro)})
    {
        const Element *e = d.element_at(in, kGamepad, axis.bit - 8);
        ASSERT_NE(e, nullptr) << axis.name;
        EXPECT_FALSE(e->constant()) << axis.name;
        EXPECT_EQ(e->bit_offset + 8, axis.bit) << axis.name;
        EXPECT_EQ(e->bit_size, 16u) << axis.name;
        EXPECT_EQ(e->logical_max, 0x3FF) << axis.name;
    }
}

// The sticks and the pressure bytes are full bytes (PS3_STICK_CENTER is 0x80, pressures go to 0xFF), so
// the logical and physical ranges are reset to 0 - 255 after the buttons, as a real DualShock 3's
// descriptor does (Logical 15 00 26 FF 00 before the pointer collection, Physical 35 00 46 FF 00 in it)
TEST(Ps3FirstPartyDescriptor, StickAndPressureRangesCoverAByte)
{
    auto d = parsed(bytes_of(kPs3FirstParty));
    const ReportType in = ReportType::Input;
    for (uint16_t axis : {kDesktopX, kDesktopY, kDesktopZ, kDesktopRz})
    {
        const Element *e = d.find(in, kGamepad, desktop(axis));
        ASSERT_NE(e, nullptr);
        EXPECT_EQ(e->logical_min, 0) << std::hex << axis;
        EXPECT_EQ(e->logical_max, 255) << std::hex << axis;
    }
    const Element *pressure = d.element_at(in, kGamepad, FIELD(PS3Gamepad_Data_t, pressureCross).bit - 8);
    ASSERT_NE(pressure, nullptr);
    EXPECT_EQ(pressure->logical_max, 255);
}

// ---------------------------------------------------------------------------------------------
// PS4

TEST(Ps4Descriptor, ReportSizes)
{
    auto d = parsed(bytes_of(kPs4));
    EXPECT_EQ(d.wire_bytes(ReportType::Input, kGamepad), sizeof(PS4Dpad_Data_t));
    EXPECT_EQ(d.wire_bytes(ReportType::Output, ReportIdPs405), sizeof(ps4_output_report));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs4Feature), 48u);
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs4SetChallenge), sizeof(AuthReport));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs4GetResponse), sizeof(AuthReport));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs4GetAuthStatus), sizeof(AuthStatusReport));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs4GetAuthPageSize), sizeof(AuthPageSizeReport));
}

TEST(Ps4Descriptor, FieldsMatchTheReportStruct)
{
    auto d = parsed(bytes_of(kPs4));
    const ReportType in = ReportType::Input;
    expect_field(d, in, kGamepad, desktop(kDesktopX), FIELD(PS4Dpad_Data_t, leftStickX), true);
    expect_field(d, in, kGamepad, desktop(kDesktopY), FIELD(PS4Dpad_Data_t, leftStickY), true);
    expect_field(d, in, kGamepad, desktop(kDesktopZ), FIELD(PS4Dpad_Data_t, rightStickX), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRz), FIELD(PS4Dpad_Data_t, rightStickY), true);
    expect_field(d, in, kGamepad, desktop(kDesktopHat), FIELD(PS4Dpad_Data_t, dpad), true);
    EXPECT_TRUE(d.find(in, kGamepad, desktop(kDesktopHat))->null_state());
    // buttons 1 - 14: square cross circle triangle L1 R1 L2 R2 share options L3 R3 PS touchpad
    expect_field(d, in, kGamepad, button(1), FIELD(PS4Dpad_Data_t, x), true);
    expect_field(d, in, kGamepad, button(2), FIELD(PS4Dpad_Data_t, a), true);
    expect_field(d, in, kGamepad, button(3), FIELD(PS4Dpad_Data_t, b), true);
    expect_field(d, in, kGamepad, button(4), FIELD(PS4Dpad_Data_t, y), true);
    expect_field(d, in, kGamepad, button(5), FIELD(PS4Dpad_Data_t, leftShoulder), true);
    expect_field(d, in, kGamepad, button(6), FIELD(PS4Dpad_Data_t, rightShoulder), true);
    expect_field(d, in, kGamepad, button(7), FIELD(PS4Dpad_Data_t, l2), true);
    expect_field(d, in, kGamepad, button(8), FIELD(PS4Dpad_Data_t, r2), true);
    expect_field(d, in, kGamepad, button(9), FIELD(PS4Dpad_Data_t, back), true);
    expect_field(d, in, kGamepad, button(10), FIELD(PS4Dpad_Data_t, start), true);
    expect_field(d, in, kGamepad, button(11), FIELD(PS4Dpad_Data_t, leftThumbClick), true);
    expect_field(d, in, kGamepad, button(12), FIELD(PS4Dpad_Data_t, rightThumbClick), true);
    expect_field(d, in, kGamepad, button(13), FIELD(PS4Dpad_Data_t, guide), true);
    expect_field(d, in, kGamepad, button(14), FIELD(PS4Dpad_Data_t, capture), true);
    expect_field(d, in, kGamepad, vendor(0x20), FIELD(PS4Dpad_Data_t, reportCounter), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRx), FIELD(PS4Dpad_Data_t, leftTrigger), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRy), FIELD(PS4Dpad_Data_t, rightTrigger), true);
    // the rest (status, motion, touchpad) is one vendor block up to the end of the struct
    auto rest = d.find_all(in, kGamepad, vendor(0x21));
    ASSERT_FALSE(rest.empty());
    EXPECT_EQ(rest.front()->bit_offset + 8, FIELD(PS4Dpad_Data_t, padding).bit);
    EXPECT_EQ((rest.back()->bit_offset + rest.back()->bit_size) / 8 + 1, sizeof(PS4Dpad_Data_t));
}

TEST(Ps4Descriptor, InstrumentReportsFitTheReport)
{
    EXPECT_EQ(sizeof(PS4Gamepad_Data_t), sizeof(PS4Dpad_Data_t));
    EXPECT_EQ(sizeof(PS4RockBandGuitar_Data_t), sizeof(PS4Dpad_Data_t));
    EXPECT_EQ(sizeof(PS4RockBandDrums_Data_t), sizeof(PS4Dpad_Data_t));
    EXPECT_EQ(sizeof(PS4GHLGuitar_Data_t), sizeof(PS4Dpad_Data_t));
}

TEST(Ps4Descriptor, OutputReportFields)
{
    // set_report reads the motors and lightbar from where a DS4 has them
    auto d = parsed(bytes_of(kPs4));
    ASSERT_TRUE(d.has_report(ReportType::Output, ReportIdPs405));
    EXPECT_LT(offsetof(ps4_output_report, lightbar_blue), d.wire_bytes(ReportType::Output, ReportIdPs405));
}

// ---------------------------------------------------------------------------------------------
// PS5

TEST(Ps5Descriptor, ReportSizes)
{
    auto d = parsed(bytes_of(kPs5));
    EXPECT_EQ(d.wire_bytes(ReportType::Input, kGamepad), sizeof(PS5Dpad_Data_t));
    EXPECT_EQ(d.wire_bytes(ReportType::Output, ReportIdPs502), sizeof(ps5_output_report));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs5Feature), 48u);
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs5SetChallenge), sizeof(AuthReport));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs5GetResponse), sizeof(AuthReport));
    EXPECT_EQ(d.wire_bytes(ReportType::Feature, ReportIdPs5GetAuthStatus), sizeof(AuthStatusReport));
}

TEST(Ps5Descriptor, FieldsMatchTheReportStruct)
{
    auto d = parsed(bytes_of(kPs5));
    const ReportType in = ReportType::Input;
    expect_field(d, in, kGamepad, desktop(kDesktopX), FIELD(PS5Dpad_Data_t, leftStickX), true);
    expect_field(d, in, kGamepad, desktop(kDesktopY), FIELD(PS5Dpad_Data_t, leftStickY), true);
    expect_field(d, in, kGamepad, desktop(kDesktopZ), FIELD(PS5Dpad_Data_t, rightStickX), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRz), FIELD(PS5Dpad_Data_t, rightStickY), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRx), FIELD(PS5Dpad_Data_t, leftTrigger), true);
    expect_field(d, in, kGamepad, desktop(kDesktopRy), FIELD(PS5Dpad_Data_t, rightTrigger), true);
    expect_field(d, in, kGamepad, vendor(0x20), FIELD(PS5Dpad_Data_t, reserved), true);
    expect_field(d, in, kGamepad, desktop(kDesktopHat), FIELD(PS5Dpad_Data_t, dpad), true);
    EXPECT_TRUE(d.find(in, kGamepad, desktop(kDesktopHat))->null_state());
    // buttons 1 - 14: square cross circle triangle L1 R1 L2 R2 create options L3 R3 PS touchpad
    expect_field(d, in, kGamepad, button(1), FIELD(PS5Dpad_Data_t, x), true);
    expect_field(d, in, kGamepad, button(2), FIELD(PS5Dpad_Data_t, a), true);
    expect_field(d, in, kGamepad, button(3), FIELD(PS5Dpad_Data_t, b), true);
    expect_field(d, in, kGamepad, button(4), FIELD(PS5Dpad_Data_t, y), true);
    expect_field(d, in, kGamepad, button(5), FIELD(PS5Dpad_Data_t, leftShoulder), true);
    expect_field(d, in, kGamepad, button(6), FIELD(PS5Dpad_Data_t, rightShoulder), true);
    expect_field(d, in, kGamepad, button(7), FIELD(PS5Dpad_Data_t, l2), true);
    expect_field(d, in, kGamepad, button(8), FIELD(PS5Dpad_Data_t, r2), true);
    expect_field(d, in, kGamepad, button(9), FIELD(PS5Dpad_Data_t, back), true);
    expect_field(d, in, kGamepad, button(10), FIELD(PS5Dpad_Data_t, start), true);
    expect_field(d, in, kGamepad, button(11), FIELD(PS5Dpad_Data_t, leftThumbClick), true);
    expect_field(d, in, kGamepad, button(12), FIELD(PS5Dpad_Data_t, rightThumbClick), true);
    expect_field(d, in, kGamepad, button(13), FIELD(PS5Dpad_Data_t, guide), true);
    expect_field(d, in, kGamepad, button(14), FIELD(PS5Dpad_Data_t, touchpad), true);
    // the vendor block from the auth sequence number to the end of the struct
    auto rest = d.find_all(in, kGamepad, vendor(0x22));
    ASSERT_FALSE(rest.empty());
    EXPECT_EQ(rest.front()->bit_offset + 8, FIELD(PS5Dpad_Data_t, auth_seq_number).bit);
    EXPECT_EQ((rest.back()->bit_offset + rest.back()->bit_size) / 8 + 1, sizeof(PS5Dpad_Data_t));
}

TEST(Ps5Descriptor, InstrumentReportsFitTheReport)
{
    // written over the PS5Dpad_Data_t buffer (PS5Gamepad_Data_t only covers the header); field positions
    // are checked in instrument_layout_test.cpp
    EXPECT_LE(sizeof(PS5Gamepad_Data_t), sizeof(PS5Dpad_Data_t));
    EXPECT_EQ(sizeof(PS5RockBandGuitar_Data_t), sizeof(PS5Dpad_Data_t));
    EXPECT_EQ(sizeof(PS5RockBandDrums_Data_t), sizeof(PS5Dpad_Data_t));
    EXPECT_EQ(sizeof(PS5GHLGuitar_Data_t), sizeof(PS5Dpad_Data_t));
}

TEST(Ps5Descriptor, OutputReportFields)
{
    auto d = parsed(bytes_of(kPs5));
    EXPECT_LT(offsetof(ps5_output_report, lightbar_blue), d.wire_bytes(ReportType::Output, ReportIdPs502));
}
