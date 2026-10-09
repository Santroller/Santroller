#include <gtest/gtest.h>
#include <vector>
#include "protocols/santroller_output.hpp"
#include "protocols/santroller_v2.hpp"
#include "protocols/stagekit.hpp"
#include "protocols/xinput.hpp"
#include "protocols/hid.hpp"
#include "protocols/ps3.hpp"

using Bytes = std::vector<uint8_t>;

namespace
{
// The next pending command, as a host would send it (report id 1, command, payload)
Bytes peek(const SantrollerOutputState &s)
{
    uint8_t buf[8];
    memset(buf, 0xAA, sizeof(buf));
    uint8_t len = s.peek(buf, sizeof(buf));
    return Bytes(buf, buf + len);
}

// Sends everything pending, in the order the state hands it out
std::vector<Bytes> drain(SantrollerOutputState &s)
{
    std::vector<Bytes> sent;
    for (Bytes cmd = peek(s); !cmd.empty(); cmd = peek(s))
    {
        sent.push_back(cmd);
        s.commit(cmd[1]);
        if (sent.size() > 10)
            break;
    }
    return sent;
}

SantrollerOutputState v1(SubType subtype)
{
    SantrollerOutputState s;
    s.subtype = subtype;
    return s;
}

SantrollerOutputState v2(SubType subtype, uint8_t caps)
{
    SantrollerOutputState s = v1(subtype);
    s.is_v2 = true;
    s.capabilities = caps;
    return s;
}
} // namespace

TEST(SantrollerOutput, CommandBytes)
{
    EXPECT_TRUE(santroller_is_command(SANTROLLER_COMMAND_RUMBLE));
    EXPECT_TRUE(santroller_is_command(SANTROLLER_COMMAND_FEEDBACK));
    EXPECT_TRUE(santroller_is_command(SANTROLLER_COMMAND_PLAYER_LED));
    EXPECT_TRUE(santroller_is_command(SANTROLLER_COMMAND_RGB_LED));
    EXPECT_FALSE(santroller_is_command(0x00));
    EXPECT_FALSE(santroller_is_command(0x01));
    EXPECT_FALSE(santroller_is_command(0x59));
    EXPECT_FALSE(santroller_is_command(0x5E));
    EXPECT_FALSE(santroller_is_command(0xFF));
    // rumble / stage kit and the expanded LED command share ids with the PS3 LED ids
    EXPECT_EQ(SANTROLLER_COMMAND_RUMBLE, SANTROLLER_LED_ID);
    EXPECT_EQ(SANTROLLER_COMMAND_FEEDBACK, SANTROLLER_LED_EXPANDED_ID);
}

TEST(SantrollerOutput, NothingPendingByDefault)
{
    SantrollerOutputState s;
    uint8_t buf[8];
    memset(buf, 0xAA, sizeof(buf));
    EXPECT_EQ(s.peek(buf, sizeof(buf)), 0);
    // the buffer is still cleared with the report id in place
    EXPECT_EQ(buf[0], 0x01);
    for (int i = 1; i < 8; i++)
        EXPECT_EQ(buf[i], 0) << i;
}

TEST(SantrollerOutput, GamepadRumble)
{
    auto s = v1(Gamepad);
    s.set_rumble(0x12, 0x34);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0x12, 0x34}));
    // peeking doesn't consume it
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0x12, 0x34}));
    s.commit(SANTROLLER_COMMAND_RUMBLE);
    EXPECT_TRUE(peek(s).empty());
    // the same values again don't need sending
    s.set_rumble(0x12, 0x34);
    EXPECT_TRUE(peek(s).empty());
    s.set_rumble(0x12, 0x35);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0x12, 0x35}));
}

TEST(SantrollerOutput, InstrumentsCarryStageKitOnTheRumbleChannel)
{
    auto s = v1(RockBandGuitar);
    s.set_rumble(0xFF, 0xFF);
    EXPECT_TRUE(peek(s).empty());
    s.set_stagekit(0x01, 0x20);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0x01, 0x20}));
    EXPECT_TRUE(s.has_stagekit());
    EXPECT_FALSE(s.has_rumble());

    auto pad = v1(Gamepad);
    pad.set_stagekit(0x01, 0x20);
    EXPECT_TRUE(peek(pad).empty());
    EXPECT_FALSE(pad.has_stagekit());
    EXPECT_TRUE(pad.has_rumble());
}

TEST(SantrollerOutput, TurntableEuphoria)
{
    auto s = v1(DjHeroTurntable);
    s.set_stagekit(0x01, 0x20);
    s.set_rumble(0x10, 0x10);
    EXPECT_TRUE(peek(s).empty());
    EXPECT_FALSE(s.has_stagekit());
    EXPECT_FALSE(s.has_rumble());
    EXPECT_TRUE(s.has_euphoria());
    s.set_euphoria(true);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0xFF, 0xFF}));
    s.commit(SANTROLLER_COMMAND_RUMBLE);
    s.set_euphoria(false);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0x00, 0x00}));

    auto pad = v1(Gamepad);
    pad.set_euphoria(true);
    EXPECT_TRUE(peek(pad).empty());
    EXPECT_FALSE(pad.has_euphoria());
}

TEST(SantrollerOutput, PlayerLedIsZeroBased)
{
    auto s = v1(Gamepad);
    for (uint8_t player = 1; player <= 4; player++)
    {
        s.set_player(player);
        EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_PLAYER_LED, uint8_t(player - 1)})) << int(player);
        s.commit(SANTROLLER_COMMAND_PLAYER_LED);
    }
    s.set_player(5);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_PLAYER_LED, 0xFF}));
    s.commit(SANTROLLER_COMMAND_PLAYER_LED);
    s.set_player(5);
    EXPECT_TRUE(peek(s).empty());
    s.set_player(0);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_PLAYER_LED, 0xFF}));
}

TEST(SantrollerOutput, RgbLed)
{
    auto s = v1(Gamepad);
    s.set_rgb(1, 2, 3);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RGB_LED, 1, 2, 3}));
    s.commit(SANTROLLER_COMMAND_RGB_LED);
    s.set_rgb(1, 2, 3);
    EXPECT_TRUE(peek(s).empty());
    s.set_rgb(1, 2, 4);
    EXPECT_FALSE(peek(s).empty());
}

TEST(SantrollerOutput, HandsOutOneCommandAtATime)
{
    auto s = v1(Gamepad);
    s.set_rgb(9, 8, 7);
    s.set_player(2);
    s.set_rumble(5, 6);
    auto sent = drain(s);
    ASSERT_EQ(sent.size(), 3u);
    EXPECT_EQ(sent[0], (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 5, 6}));
    EXPECT_EQ(sent[1], (Bytes{0x01, SANTROLLER_COMMAND_PLAYER_LED, 1}));
    EXPECT_EQ(sent[2], (Bytes{0x01, SANTROLLER_COMMAND_RGB_LED, 9, 8, 7}));
}

TEST(SantrollerOutput, CommitOnlyClearsThatCommand)
{
    auto s = v1(Gamepad);
    s.set_rumble(5, 6);
    s.commit(SANTROLLER_COMMAND_PLAYER_LED);
    s.commit(SANTROLLER_COMMAND_FEEDBACK);
    s.commit(0x00);
    EXPECT_EQ(peek(s), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 5, 6}));
}

TEST(SantrollerOutput, MarkAllDirtyResendsEverything)
{
    auto s = v1(Gamepad);
    s.set_rumble(5, 6);
    s.set_player(3);
    s.set_rgb(1, 1, 1);
    drain(s);
    EXPECT_TRUE(peek(s).empty());
    s.mark_all_dirty();
    auto sent = drain(s);
    ASSERT_EQ(sent.size(), 3u);
    EXPECT_EQ(sent[0], (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 5, 6}));
    EXPECT_EQ(sent[1], (Bytes{0x01, SANTROLLER_COMMAND_PLAYER_LED, 2}));
    EXPECT_EQ(sent[2], (Bytes{0x01, SANTROLLER_COMMAND_RGB_LED, 1, 1, 1}));
}

TEST(SantrollerOutput, Santroller1IsSentEverything)
{
    // no capabilities report, so every command is sent
    auto s = v1(Gamepad);
    EXPECT_TRUE(s.supports(0));
    EXPECT_TRUE(s.supports(CapabilityHasRumble));
    EXPECT_TRUE(s.supports(0xFF));
}

TEST(SantrollerOutput, Santroller2OnlyGetsWhatItSupports)
{
    auto s = v2(Gamepad, CapabilityHasStandardPlayerLeds);
    s.set_rumble(5, 6);
    s.set_player(1);
    s.set_rgb(1, 2, 3);
    auto sent = drain(s);
    ASSERT_EQ(sent.size(), 1u);
    EXPECT_EQ(sent[0], (Bytes{0x01, SANTROLLER_COMMAND_PLAYER_LED, 0}));
    EXPECT_FALSE(s.has_rumble());

    auto rumble = v2(Gamepad, CapabilityHasRumble);
    rumble.set_rumble(5, 6);
    EXPECT_EQ(peek(rumble), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 5, 6}));
    EXPECT_TRUE(rumble.has_rumble());

    auto rgb = v2(Gamepad, CapabilityHasRGBIndicatorLed);
    rgb.set_rgb(1, 2, 3);
    EXPECT_EQ(peek(rgb), (Bytes{0x01, SANTROLLER_COMMAND_RGB_LED, 1, 2, 3}));
}

TEST(SantrollerOutput, Santroller2InstrumentsNeedInstrumentLeds)
{
    auto plain = v2(RockBandDrums, CapabilityHasRumble);
    plain.set_stagekit(1, 2);
    EXPECT_TRUE(peek(plain).empty());
    EXPECT_FALSE(plain.has_stagekit());

    auto leds = v2(RockBandDrums, CapabilityHasInstrumentLeds);
    leds.set_stagekit(1, 2);
    EXPECT_EQ(peek(leds), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 1, 2}));
    EXPECT_TRUE(leds.has_stagekit());
}

TEST(SantrollerOutput, Santroller2TurntableEuphoriaIsTheRgbLed)
{
    auto none = v2(DjHeroTurntable, CapabilityHasInstrumentLeds | CapabilityHasRumble);
    none.set_euphoria(true);
    EXPECT_TRUE(peek(none).empty());
    EXPECT_FALSE(none.has_euphoria());

    auto rgb = v2(DjHeroTurntable, CapabilityHasRGBIndicatorLed);
    rgb.set_euphoria(true);
    EXPECT_EQ(peek(rgb), (Bytes{0x01, SANTROLLER_COMMAND_RUMBLE, 0xFF, 0xFF}));
    EXPECT_TRUE(rgb.has_euphoria());
}

TEST(Stagekit, InstrumentSubtypesCarryStageKitCommands)
{
    for (SubType t : {StageKit, GuitarHeroDrums, RockBandDrums, GuitarHeroGuitar, RockBandGuitar, DjHeroTurntable})
        EXPECT_TRUE(subtype_supports_stagekit(t)) << t;
    for (SubType t : {Gamepad, Dancepad, Wheel, FightStick, FlightStick, KeyboardMouse, Unknown})
        EXPECT_FALSE(subtype_supports_stagekit(t)) << t;
}

TEST(SantrollerV2, Ids)
{
    // pid.codes vendor id, Santroller's pid.codes product id
    EXPECT_EQ(SANTROLLER_VID, 0x1209);
    EXPECT_EQ(SANTROLLER_PID, 0x2882);
}

TEST(SantrollerV2, HatBecomesXInputDpadBits)
{
    // HID hat: 0 = up, clockwise, anything else neutral. XInput dpad: up 1, down 2, left 4, right 8
    const uint8_t expected[16] = {0x01, 0x09, 0x08, 0x0A, 0x02, 0x06, 0x04, 0x05, 0, 0, 0, 0, 0, 0, 0, 0};
    for (uint8_t hat = 0; hat < 16; hat++)
    {
        uint8_t report[20] = {0x01, 0x14, hat, 0x00};
        santroller_v2_normalize_report(report, sizeof(report), Gamepad);
        EXPECT_EQ(report[2], expected[hat]) << int(hat);
    }
}

TEST(SantrollerV2, NormalisedDpadReadsAsXInput)
{
    uint8_t report[sizeof(XInputGamepad_Data_t)] = {0x01, 0x14, 0x03};
    santroller_v2_normalize_report(report, sizeof(report), Gamepad);
    XInputGamepad_Data_t parsed;
    memcpy(&parsed, report, sizeof(parsed));
    EXPECT_FALSE(parsed.dpadUp);
    EXPECT_TRUE(parsed.dpadDown);
    EXPECT_FALSE(parsed.dpadLeft);
    EXPECT_TRUE(parsed.dpadRight);
}

TEST(SantrollerV2, KeepsTheButtonsInTheHighNibble)
{
    uint8_t report[20] = {0x01, 0x14, 0xF6, 0xFF};
    santroller_v2_normalize_report(report, sizeof(report), Gamepad);
    EXPECT_EQ(report[2], 0xF4);
    EXPECT_EQ(report[3], 0xFF);
    EXPECT_EQ(report[0], 0x01);
    EXPECT_EQ(report[1], 0x14);
}

TEST(SantrollerV2, DancepadIsAlreadyABitmask)
{
    uint8_t report[20] = {0x01, 0x14, 0x0F};
    santroller_v2_normalize_report(report, sizeof(report), Dancepad);
    EXPECT_EQ(report[2], 0x0F);
}

TEST(SantrollerV2, ShortReportsAreLeftAlone)
{
    uint8_t report[3] = {0x01, 0x14, 0x00};
    santroller_v2_normalize_report(report, 2, Gamepad);
    EXPECT_EQ(report[2], 0x00);
    santroller_v2_normalize_report(report, 0, Gamepad);
    EXPECT_EQ(report[2], 0x00);
    santroller_v2_normalize_report(report, 3, Gamepad);
    EXPECT_EQ(report[2], 0x01);
}

// The Santroller 2 HID gamepad report mirrors XInput's, except for the hat in the low nibble of byte 2
static_assert(sizeof(PCGamepadDpad_Data_t) == sizeof(XInputGamepad_Data_t));
static_assert(offsetof(PCGamepadDpad_Data_t, leftTrigger) == offsetof(XInputGamepad_Data_t, leftTrigger));
static_assert(offsetof(PCGamepadDpad_Data_t, rightTrigger) == offsetof(XInputGamepad_Data_t, rightTrigger));
static_assert(offsetof(PCGamepadDpad_Data_t, leftStickX) == offsetof(XInputGamepad_Data_t, leftStickX));
static_assert(offsetof(PCGamepadDpad_Data_t, rightStickY) == offsetof(XInputGamepad_Data_t, rightStickY));

TEST(SantrollerV2, ButtonsMatchXInputBits)
{
    // everything but the dpad sits on the same bit as XInput
    auto same = [](auto set_pc, auto set_xinput) {
        PCGamepadDpad_Data_t pc;
        XInputGamepad_Data_t xi;
        memset(&pc, 0, sizeof(pc));
        memset(&xi, 0, sizeof(xi));
        set_pc(pc);
        set_xinput(xi);
        return memcmp(&pc, &xi, sizeof(pc)) == 0;
    };
#define SAME_BIT(f) EXPECT_TRUE(same([](auto &r) { r.f = 1; }, [](auto &r) { r.f = 1; })) << #f
    SAME_BIT(start);
    SAME_BIT(back);
    SAME_BIT(leftThumbClick);
    SAME_BIT(rightThumbClick);
    SAME_BIT(leftShoulder);
    SAME_BIT(rightShoulder);
    SAME_BIT(guide);
    SAME_BIT(capture);
    SAME_BIT(a);
    SAME_BIT(b);
    SAME_BIT(x);
    SAME_BIT(y);
#undef SAME_BIT
}
