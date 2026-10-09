#include <gtest/gtest.h>
#include <vector>
#include "protocols/xbox_hid.hpp"
#include "protocols/proto_output.hpp"

// Xbox One S / Elite 2 / Series controllers in HID mode. Layouts follow SDL's HIDAPI Xbox One
// Bluetooth driver and Linux hid-microsoft, as the header says.

namespace
{
using Bytes = std::vector<uint8_t>;

// Report 0x01 in the newer (Android gamepad) layout: sticks, 10 bit triggers, hat, buttons, extra byte
Bytes input_report(uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry, uint16_t lt, uint16_t rt, uint8_t hat,
                   uint8_t b14, uint8_t b15, uint8_t b16, size_t len = 17)
{
    Bytes r(len, 0);
    r[0] = XBOX_HID_INPUT_REPORT_ID;
    auto put = [&](size_t at, uint16_t v) { r[at] = v & 0xFF; r[at + 1] = v >> 8; };
    put(1, lx);
    put(3, ly);
    put(5, rx);
    put(7, ry);
    put(9, lt);
    put(11, rt);
    r[13] = hat;
    r[14] = b14;
    r[15] = b15;
    if (len > 16)
        r[16] = b16;
    return r;
}

XboxHidState parse(const Bytes &r, bool has_share = false, XboxHidState state = {})
{
    xbox_hid_parse_fixed(r.data(), r.size(), has_share, state);
    return state;
}

uint16_t trigger(uint8_t lo, uint8_t hi)
{
    const uint8_t buf[2] = {lo, hi};
    return xbox_hid_trigger(buf);
}

bool pressed(const XboxHidState &s, GamepadButtonType b)
{
    proto_Output o = gamepad_button(b);
    return xbox_hid_tick_digital(s, o);
}

uint16_t axis(const XboxHidState &s, GamepadAxisType a)
{
    proto_Output o = gamepad_axis(a);
    return xbox_hid_tick_analog(s, o);
}

// RAII owner for a parsed descriptor so the parser's shared item pool is released between tests
struct ParsedDescriptor
{
    HID_ReportInfo_t *info = nullptr;
    explicit ParsedDescriptor(const Bytes &desc)
    {
        EXPECT_EQ(USB_ProcessHIDReport(desc.data(), desc.size(), &info), HID_PARSE_Successful);
    }
};

// A third party controller that puts Share (Consumer Record) at bit 2 of byte 17 instead of byte 16
const Bytes kShareAtByte17 = {
    0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x01,
    0x75, 0x08, 0x95, 0x10, 0x81, 0x03,                                     // bytes 1 - 16, constant
    0x75, 0x02, 0x95, 0x01, 0x81, 0x03,                                     // 2 bits of padding
    0x05, 0x0C, 0x0A, 0xB2, 0x00, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x01, 0x81, 0x02, // Record
    0x75, 0x05, 0x95, 0x01, 0x81, 0x03,
    0xC0};

// Same report without any Record usage
const Bytes kNoShare = {
    0x05, 0x01, 0x09, 0x05, 0xA1, 0x01, 0x85, 0x01,
    0x09, 0x30, 0x09, 0x31, 0x15, 0x00, 0x27, 0xFF, 0xFF, 0x00, 0x00, 0x75, 0x10, 0x95, 0x02, 0x81, 0x02,
    0xC0};
} // namespace

TEST(XboxHid, RecognisesTheMicrosoftHidPids)
{
    // PIDs from SDL's list of Xbox One controllers that speak HID over Bluetooth
    EXPECT_TRUE(is_xbox_hid_controller(0x045E, 0x02E0)); // One S, original Bluetooth firmware
    EXPECT_TRUE(is_xbox_hid_controller(0x045E, 0x02FD)); // One S
    EXPECT_TRUE(is_xbox_hid_controller(0x045E, 0x0B05)); // Elite 2
    EXPECT_TRUE(is_xbox_hid_controller(0x045E, 0x0B13)); // Series X|S
    EXPECT_TRUE(is_xbox_hid_controller(0x045E, 0x0B20)); // One S BLE
    EXPECT_TRUE(is_xbox_hid_controller(0x045E, 0x0B22)); // Elite 2 BLE
}

TEST(XboxHid, RejectsOtherDevices)
{
    EXPECT_FALSE(is_xbox_hid_controller(0x045E, 0x02EA)); // One S over USB speaks GIP, not HID
    EXPECT_FALSE(is_xbox_hid_controller(0x045E, 0x028E)); // 360 controller
    EXPECT_FALSE(is_xbox_hid_controller(0x054C, 0x0B13)); // right PID, wrong vendor
    EXPECT_FALSE(is_xbox_hid_controller(0, 0));
}

TEST(XboxHid, DefaultStateIsNeutral)
{
    XboxHidState s{};
    EXPECT_EQ(axis(s, Gamepad_LeftStickX), 0x8000);
    EXPECT_EQ(axis(s, Gamepad_RightStickX), 0x8000);
    // Y is inverted, so the centre comes out one below 0x8000
    EXPECT_EQ(axis(s, Gamepad_LeftStickY), 0x7FFF);
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0);
    for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
        EXPECT_FALSE(pressed(s, (GamepadButtonType)b)) << b;
}

TEST(XboxHid, ParsesSticksLittleEndian)
{
    auto s = parse(input_report(0x1234, 0xABCD, 0x0001, 0xFFFF, 0, 0, 0, 0, 0, 0));
    EXPECT_EQ(s.lx, 0x1234);
    EXPECT_EQ(s.ly, 0xABCD);
    EXPECT_EQ(s.rx, 0x0001);
    EXPECT_EQ(s.ry, 0xFFFF);
}

TEST(XboxHid, YAxesAreInvertedSoUpIsTheMaximum)
{
    // HID reports down as the maximum
    auto down = parse(input_report(0x8000, 0xFFFF, 0x8000, 0xFFFF, 0, 0, 0, 0, 0, 0));
    EXPECT_EQ(axis(down, Gamepad_LeftStickY), 0);
    EXPECT_EQ(axis(down, Gamepad_RightStickY), 0);
    auto up = parse(input_report(0x8000, 0, 0x8000, 0, 0, 0, 0, 0, 0, 0));
    EXPECT_EQ(axis(up, Gamepad_LeftStickY), 0xFFFF);
    EXPECT_EQ(axis(up, Gamepad_RightStickY), 0xFFFF);
    // X is passed straight through
    auto right = parse(input_report(0xFFFF, 0x8000, 0, 0x8000, 0, 0, 0, 0, 0, 0));
    EXPECT_EQ(axis(right, Gamepad_LeftStickX), 0xFFFF);
    EXPECT_EQ(axis(right, Gamepad_RightStickX), 0);
}

TEST(XboxHid, TriggersScaleTenBitsToFullRange)
{
    EXPECT_EQ(trigger(0x00, 0x00), 0);
    EXPECT_EQ(trigger(0xFF, 0x03), 0xFFFF);
    // half way: 0x1FF / 0x3FF of full scale
    EXPECT_EQ(trigger(0xFF, 0x01), 0x1FF * 0xFFFF / 0x3FF);
    // anything above 10 bits is ignored
    EXPECT_EQ(trigger(0xFF, 0xFF), 0xFFFF);
    EXPECT_EQ(trigger(0x00, 0xFC), 0);

    auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0x3FF, 0x000, 0, 0, 0, 0));
    EXPECT_EQ(axis(s, Gamepad_LeftTrigger), 0xFFFF);
    EXPECT_EQ(axis(s, Gamepad_RightTrigger), 0);
}

TEST(XboxHid, HatIsClockwiseFromUpWithZeroNeutral)
{
    struct
    {
        uint8_t hat;
        bool up, right, down, left;
    } cases[] = {
        {0, false, false, false, false},
        {1, true, false, false, false},
        {2, true, true, false, false},
        {3, false, true, false, false},
        {4, false, true, true, false},
        {5, false, false, true, false},
        {6, false, false, true, true},
        {7, false, false, false, true},
        {8, true, false, false, true},
        {9, false, false, false, false},
        {0x0F, false, false, false, false},
    };
    for (auto c : cases)
    {
        auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, c.hat, 0, 0, 0));
        EXPECT_EQ(pressed(s, Gamepad_DpadUp), c.up) << int(c.hat);
        EXPECT_EQ(pressed(s, Gamepad_DpadRight), c.right) << int(c.hat);
        EXPECT_EQ(pressed(s, Gamepad_DpadDown), c.down) << int(c.hat);
        EXPECT_EQ(pressed(s, Gamepad_DpadLeft), c.left) << int(c.hat);
    }
}

TEST(XboxHid, AndroidLayoutButtons)
{
    // byte 14: A B (C) X Y (Z) LB RB; byte 15: (L2 R2) Back Start Guide LS RS
    struct
    {
        uint8_t b14, b15;
        GamepadButtonType button;
    } cases[] = {
        {0x01, 0, Gamepad_A},         {0x02, 0, Gamepad_B},          {0x08, 0, Gamepad_X},
        {0x10, 0, Gamepad_Y},         {0x40, 0, Gamepad_LeftShoulder}, {0x80, 0, Gamepad_RightShoulder},
        {0, 0x04, Gamepad_Back},      {0, 0x08, Gamepad_Start},      {0, 0x10, Gamepad_Guide},
        {0, 0x20, Gamepad_LeftThumbClick}, {0, 0x40, Gamepad_RightThumbClick},
    };
    for (auto c : cases)
    {
        auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, c.b14, c.b15, 0));
        for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
            EXPECT_EQ(pressed(s, (GamepadButtonType)b), b == c.button)
                << "bytes " << int(c.b14) << "/" << int(c.b15) << " button " << b;
    }
}

TEST(XboxHid, AndroidLayoutGapsAreIgnored)
{
    // C, Z, L2 and R2 positions don't exist on an Xbox controller
    auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0x04 | 0x20, 0x01 | 0x02 | 0x80, 0));
    for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
        EXPECT_FALSE(pressed(s, (GamepadButtonType)b)) << b;
}

TEST(XboxHid, LegacyOneSLayoutIsPackedInOrder)
{
    // 16 byte report from the original One S Bluetooth firmware
    struct
    {
        uint8_t b14, b15;
        GamepadButtonType button;
    } cases[] = {
        {0x01, 0, Gamepad_A},     {0x02, 0, Gamepad_B},     {0x04, 0, Gamepad_X},
        {0x08, 0, Gamepad_Y},     {0x10, 0, Gamepad_LeftShoulder}, {0x20, 0, Gamepad_RightShoulder},
        {0x40, 0, Gamepad_Back},  {0x80, 0, Gamepad_Start}, {0, 0x01, Gamepad_LeftThumbClick},
        {0, 0x02, Gamepad_RightThumbClick},
    };
    for (auto c : cases)
    {
        auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, c.b14, c.b15, 0, 16));
        for (int b = Gamepad_A; b <= Gamepad_DpadRight; b++)
            EXPECT_EQ(pressed(s, (GamepadButtonType)b), b == c.button)
                << "bytes " << int(c.b14) << "/" << int(c.b15) << " button " << b;
    }
}

TEST(XboxHid, LegacyGuideComesFromReport2)
{
    XboxHidState s{};
    Bytes guide = {XBOX_HID_GUIDE_REPORT_ID, 0x01};
    s = parse(guide, false, s);
    EXPECT_TRUE(s.has_guide_report);
    EXPECT_TRUE(pressed(s, Gamepad_Guide));
    // Input reports don't touch guide once the guide report has shown up, even in the newer layout
    s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0, 0, 0), false, s);
    EXPECT_TRUE(pressed(s, Gamepad_Guide));
    s = parse(Bytes{XBOX_HID_GUIDE_REPORT_ID, 0x00}, false, s);
    EXPECT_FALSE(pressed(s, Gamepad_Guide));
    s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0, 0x10, 0), false, s);
    EXPECT_FALSE(pressed(s, Gamepad_Guide));
}

TEST(XboxHid, ExtraByteIsShareWhenTheControllerHasOne)
{
    auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0, 0, 0x01), true);
    EXPECT_TRUE(pressed(s, Gamepad_Capture));
    EXPECT_FALSE(pressed(s, Gamepad_Back));
}

TEST(XboxHid, ExtraByteIsBackWithoutShare)
{
    // One S / Elite 2 BLE firmware sends View as AC Back in byte 16
    auto s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0, 0, 0x01), false);
    EXPECT_FALSE(pressed(s, Gamepad_Capture));
    EXPECT_TRUE(pressed(s, Gamepad_Back));
    // and it doesn't hide the normal View bit
    s = parse(input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0, 0x04, 0x00), false);
    EXPECT_TRUE(pressed(s, Gamepad_Back));
}

TEST(XboxHid, ShortAndUnknownReportsLeaveTheStateAlone)
{
    XboxHidState before{};
    before.a = true;
    before.lx = 0x1234;
    for (size_t len : {0u, 1u, 2u, 15u})
    {
        Bytes r = input_report(0, 0, 0, 0, 0x3FF, 0x3FF, 1, 0xFF, 0xFF, 0xFF);
        r.resize(len);
        auto s = parse(r, false, before);
        EXPECT_TRUE(s.a) << len;
        EXPECT_EQ(s.lx, 0x1234) << len;
        EXPECT_FALSE(s.up) << len;
    }
    Bytes other = input_report(0, 0, 0, 0, 0x3FF, 0x3FF, 1, 0xFF, 0xFF, 0xFF);
    other[0] = XBOX_HID_RUMBLE_REPORT_ID;
    auto s = parse(other, false, before);
    EXPECT_EQ(s.lx, 0x1234);
    EXPECT_FALSE(s.up);
}

TEST(XboxHid, TickHelpersIgnoreOtherMappings)
{
    XboxHidState s{};
    s.a = true;
    s.lx = 0xFFFF;
    proto_Output button = non_gamepad_mapping(Gamepad_A);
    proto_Output ax = non_gamepad_mapping(Gamepad_LeftStickX);
    EXPECT_FALSE(xbox_hid_tick_digital(s, button));
    EXPECT_EQ(xbox_hid_tick_analog(s, ax), 0);
    // an axis mapping isn't a button and vice versa
    proto_Output as_axis = gamepad_axis(Gamepad_LeftStickX);
    EXPECT_FALSE(xbox_hid_tick_digital(s, as_axis));
    proto_Output as_button = gamepad_button(Gamepad_A);
    EXPECT_EQ(xbox_hid_tick_analog(s, as_button), 0);
    // axes the controller doesn't have
    EXPECT_EQ(axis(s, Gamepad_AccelX), 0);
    EXPECT_EQ(axis(s, Gamepad_Gyro), 0);
}

TEST(XboxHid, DescriptorWithoutRecordOnlyTrustsTheSeriesPid)
{
    {
        XboxHidDescriptor desc;
        desc.init(nullptr, XBOX_SERIES_BT_PID);
        EXPECT_TRUE(desc.has_share);
        EXPECT_FALSE(desc.share_in_descriptor);
    }
    {
        XboxHidDescriptor desc;
        desc.init(nullptr, XBOX_ELITE_2_BT_PID);
        EXPECT_FALSE(desc.has_share);
    }
    {
        ParsedDescriptor parsed(kNoShare);
        XboxHidDescriptor desc;
        desc.init(parsed.info, XBOX_ONE_S_BLE_PID);
        EXPECT_FALSE(desc.has_share);
        EXPECT_FALSE(desc.share_in_descriptor);
        // the parsed items aren't needed, so they've been released already
        EXPECT_EQ(desc.info, nullptr);
    }
}

TEST(XboxHid, ShareIsReadWhereverTheDescriptorPutsIt)
{
    ParsedDescriptor parsed(kShareAtByte17);
    XboxHidDescriptor desc;
    desc.init(parsed.info, XBOX_ONE_S_BLE_PID);
    EXPECT_TRUE(desc.share_in_descriptor);
    EXPECT_TRUE(desc.has_share);
    ASSERT_NE(desc.info, nullptr);

    XboxHidState s{};
    // byte 16 set, byte 17 clear: the descriptor says share isn't pressed
    Bytes r = input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0, 0, 0x01, 18);
    xbox_hid_parse_report(r.data(), r.size(), desc, s);
    EXPECT_FALSE(pressed(s, Gamepad_Capture));
    // bit 2 of byte 17 is share
    r[16] = 0;
    r[17] = 0x04;
    xbox_hid_parse_report(r.data(), r.size(), desc, s);
    EXPECT_TRUE(pressed(s, Gamepad_Capture));
    // the fixed layout is still used for everything else
    r[14] = 0x01;
    xbox_hid_parse_report(r.data(), r.size(), desc, s);
    EXPECT_TRUE(pressed(s, Gamepad_A));
}

TEST(XboxHid, ParseReportWithoutDescriptorUsesTheFixedLayout)
{
    XboxHidDescriptor desc;
    desc.init(nullptr, XBOX_SERIES_BT_PID);
    XboxHidState s{};
    Bytes r = input_report(0x8000, 0x8000, 0x8000, 0x8000, 0, 0, 0, 0x01, 0, 0x01);
    xbox_hid_parse_report(r.data(), r.size(), desc, s);
    EXPECT_TRUE(pressed(s, Gamepad_A));
    EXPECT_TRUE(pressed(s, Gamepad_Capture));
}

TEST(XboxHid, RumblePayloadMatchesHidMicrosoft)
{
    // enable mask, LT, RT, strong (left), weak (right) magnitudes 0 - 100, duration, delay, loop count
    uint8_t out[XBOX_HID_RUMBLE_LEN];
    memset(out, 0xAA, sizeof(out));
    xbox_hid_build_rumble(255, 0, out);
    const uint8_t expected_full_left[] = {0x03, 0, 0, 100, 0, 0xFF, 0, 0};
    EXPECT_EQ(0, memcmp(out, expected_full_left, sizeof(out)));

    xbox_hid_build_rumble(0, 255, out);
    EXPECT_EQ(out[3], 0);
    EXPECT_EQ(out[4], 100);

    xbox_hid_build_rumble(0, 0, out);
    EXPECT_EQ(out[3], 0);
    EXPECT_EQ(out[4], 0);
    // still enables both motors so the zero magnitudes stop them
    EXPECT_EQ(out[0], XBOX_HID_RUMBLE_ENABLE_WEAK | XBOX_HID_RUMBLE_ENABLE_STRONG);

    xbox_hid_build_rumble(128, 64, out);
    EXPECT_EQ(out[3], 128 * 100 / 255);
    EXPECT_EQ(out[4], 64 * 100 / 255);
    // trigger motors stay off
    EXPECT_EQ(out[1], 0);
    EXPECT_EQ(out[2], 0);
}

TEST(XboxHidRumble, FirstSendWaitsForTheMinimumInterval)
{
    XboxHidRumble r;
    EXPECT_TRUE(r.dirty);
    EXPECT_FALSE(r.should_send(0));
    EXPECT_FALSE(r.should_send(XBOX_HID_OUTPUT_MIN_INTERVAL_MS - 1));
    EXPECT_TRUE(r.should_send(XBOX_HID_OUTPUT_MIN_INTERVAL_MS));
}

TEST(XboxHidRumble, ChangesAreRateLimited)
{
    XboxHidRumble r;
    r.sent(100);
    EXPECT_FALSE(r.should_send(200));
    r.set(10, 20);
    EXPECT_TRUE(r.dirty);
    r.sent(200);
    r.set(30, 20);
    EXPECT_FALSE(r.should_send(205));
    EXPECT_TRUE(r.should_send(210));
}

TEST(XboxHidRumble, SameValuesDontDirty)
{
    XboxHidRumble r;
    r.set(10, 20);
    r.sent(50);
    r.set(10, 20);
    EXPECT_FALSE(r.dirty);
}

TEST(XboxHidRumble, HeldRumbleIsRefreshed)
{
    XboxHidRumble r;
    r.set(10, 0);
    r.sent(1000);
    EXPECT_FALSE(r.should_send(1000 + XBOX_HID_RUMBLE_REFRESH_MS - 1));
    EXPECT_TRUE(r.should_send(1000 + XBOX_HID_RUMBLE_REFRESH_MS));
    // the refresh is well inside the 2.55 second duration the payload asks for
    EXPECT_LT(XBOX_HID_RUMBLE_REFRESH_MS, 2550);
}

TEST(XboxHidRumble, StoppedRumbleIsNotRefreshed)
{
    XboxHidRumble r;
    r.set(0, 0);
    r.sent(1000);
    EXPECT_FALSE(r.should_send(1000 + 10 * XBOX_HID_RUMBLE_REFRESH_MS));
}

TEST(XboxHidRumble, SurvivesMillisecondWraparound)
{
    XboxHidRumble r;
    r.set(5, 5);
    r.sent(UINT32_MAX - 4);
    EXPECT_FALSE(r.should_send(2)); // 7ms later
    r.set(6, 6);
    EXPECT_TRUE(r.should_send(5)); // 10ms later
}
