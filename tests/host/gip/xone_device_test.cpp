// src/emulation/usb/xone_device.cpp: the emulated Xbox One controller, played from the console's
// side through the fake TinyUSB device stack in support/gip. The static Hello / metadata tables it
// sends are also checked directly against [MS-GIPUSB] and the real devices they imitate.
#include <gtest/gtest.h>

#include "emulation/usb/xone_device.h"
#include "gip_device_mappings.h"
#include "gip_fakes.hpp"
#include "gip_packet_handler.h"
#include "gip_test_support.hpp"
#include "pico/time.h"
#include "plasticband_metadata.hpp"
#include "usb/auth_broker.h"
#include "xgip_protocol.h"

using namespace gip_test;
namespace pb = gip_test::plasticband;

namespace
{
constexpr uint8_t EP_OUT = 0x01;
constexpr uint8_t EP_IN = 0x81;
const uint8_t GIP_INTERFACE[] = {
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x47, 0xD0, 0x00,
    0x07, 0x05, EP_OUT, 0x03, 0x40, 0x00, 0x01,
    0x07, 0x05, EP_IN, 0x03, 0x40, 0x00, 0x01,
};
const char *SOURCE = "src/emulation/usb/xone_device.cpp";

struct Emulated
{
    SubType subtype;
    const char *announce;
    const char *descriptor;
    uint16_t vid, pid;          // the real device it presents as (PlasticBand / [MS-GIPUSB])
    const char *preferred_type; // its class string
    SubType detected;           // what lib/gip_common makes of the metadata
    size_t report_size;         // size of the 0x20 report the emulation sends
};

// VID / PID and class strings: PlasticBand Instruments/*/Xbox One.md (Riffmaster 0E6F:0248,
// MadCatz drums 0738:4262, GHL 1430:079B) and Microsoft's Xbox One S pad 045E:02EA
const Emulated EMULATED[] = {
    {Gamepad, "announce_gamepad", "xb1_descriptor_gamepad", 0x045E, 0x02EA, "Windows.Xbox.Input.Gamepad", Gamepad, sizeof(XboxOneGamepad_Data_t)},
    {RockBandGuitar, "announce_guitar", "xb1_descriptor_guitar", 0x0E6F, 0x0248, "MadCatz.Xbox.Guitar.Stratocaster", RockBandGuitar, sizeof(XboxOneRockBandGuitar_Data_t)},
    {RockBandDrums, "announce_drum", "xb1_descriptor_drum", 0x0738, 0x4262, "MadCatz.Xbox.Drums.Glam", RockBandDrums, sizeof(XboxOneRockBandDrums_Data_t)},
    {LiveGuitar, "announce_ghl", "xb1_descriptor_ghl", 0x1430, 0x079B, "Activision.Xbox.Input.GH7", LiveGuitar, sizeof(XboxOneGHLGuitar_Data_t)},
};

uint16_t le16(const Bytes &b, size_t at)
{
    return b[at] | b[at + 1] << 8;
}

// The length a metadata blob declares for a message (0x20, the input report, by default), 0 if none
uint16_t declared_input_length(const Bytes &md, uint8_t message = 0x20)
{
    const uint8_t *dm = md.data() + 16;
    uint16_t msgs = dm[0] | dm[1] << 8;
    uint8_t count = dm[msgs];
    for (uint8_t i = 0; i < count; i++)
    {
        const uint8_t *e = dm + msgs + 1 + i * 23;
        if (e[2] == message)
            return e[3] | e[4] << 8;
    }
    return 0;
}

class XoneDevice : public ::testing::Test
{
protected:
    std::unique_ptr<XboxOneGamepadDevice> dev;
    uint8_t console_seq = 0;

    void SetUp() override
    {
        gip_fake::reset();
    }
    void TearDown() override
    {
        dev.reset();
        gip_fake::reset();
    }
    void plug(SubType subtype, bool with_profile = true)
    {
        dev = std::make_unique<XboxOneGamepadDevice>();
        dev->subtype = subtype;
        if (with_profile)
        {
            auto profile = std::make_shared<Profile>();
            profile->subtype = subtype;
            mapping = std::make_shared<FakeXboxOneMapping>();
            profile->mappings.push_back(mapping);
            dev->profiles.push_back(profile);
        }
        ASSERT_EQ(dev->open((tusb_desc_interface_t const *)GIP_INTERFACE, sizeof(GIP_INTERFACE)), sizeof(GIP_INTERFACE));
    }
    // The console sends one transfer
    void from_console(const Bytes &wire)
    {
        ASSERT_LE(wire.size(), sizeof(dev->epout_buf));
        memcpy(dev->epout_buf, wire.data(), wire.size());
        dev->interrupt_xfer(EP_OUT, XFER_RESULT_SUCCESS, wire.size());
    }
    uint8_t next_seq()
    {
        if (++console_seq == 0)
            console_seq = 1;
        return console_seq;
    }
    // Run the device's main loop for `ms`, returning what it sent to the console
    std::vector<Bytes> run(uint32_t ms)
    {
        for (uint32_t i = 0; i < ms; i++)
        {
            gip_fake_time::advance_ms(1);
            dev->process(false, false);
        }
        std::vector<Bytes> sent;
        for (auto &t : gip_fake::device_in)
        {
            EXPECT_EQ(t.ep, EP_IN);
            sent.push_back(t.data);
        }
        gip_fake::device_in.clear();
        return sent;
    }
    static std::vector<Bytes> of_type(const std::vector<Bytes> &packets, uint8_t type)
    {
        std::vector<Bytes> out;
        for (auto &p : packets)
            if (!p.empty() && p[0] == type)
                out.push_back(p);
        return out;
    }
    // Bring the device up the short way: START with no controller to relay auth to means the
    // device considers itself authenticated and starts sending input
    void start()
    {
        from_console(packet(GIP_SET_STATE, FLAG_SYSTEM, next_seq(), {GIP_STATE_START}));
        run(50);
    }

    std::shared_ptr<FakeXboxOneMapping> mapping;
};
} // namespace

// --- The static tables ---

// [MS-GIPUSB] Table 27 (Hello Device): 28 byte payload; Device ID is 8 bytes with the top two 0;
// VID / PID little-endian; RF, security and GIP protocol versions all MUST be 1.0
TEST(XoneDeviceTables, HelloPayloads)
{
    for (auto &e : EMULATED)
    {
        SCOPED_TRACE(e.announce);
        Bytes hello = firmware_table(SOURCE, e.announce);
        ASSERT_EQ(hello.size(), 28u);
        EXPECT_EQ(hello[6], 0x00);
        EXPECT_EQ(hello[7], 0x00);
        EXPECT_EQ(le16(hello, 8), e.vid);
        EXPECT_EQ(le16(hello, 10), e.pid);
        EXPECT_EQ(Bytes(hello.begin() + 22, hello.end()), (Bytes{0x01, 0x00, 0x01, 0x00, 0x01, 0x00}));
    }
}

// [MS-GIPUSB] 2.2.2: "At least one of the supported Major and Minor Firmware Version pairs listed in
// metadata MUST match what is provided by the Hello message or the host will stop responding"
TEST(XoneDeviceTables, HelloFirmwareVersionIsInTheMetadata)
{
    for (auto &e : EMULATED)
    {
        SCOPED_TRACE(e.announce);
        Bytes hello = firmware_table(SOURCE, e.announce);
        Bytes md = firmware_table(SOURCE, e.descriptor);
        ASSERT_GT(md.size(), 40u);
        uint16_t major = le16(hello, 12), minor = le16(hello, 14);
        size_t fw = 16 + le16(md, 16 + 2);
        uint8_t count = md[fw];
        bool found = false;
        for (uint8_t i = 0; i < count; i++)
            found |= le16(md, fw + 1 + i * 4) == major && le16(md, fw + 3 + i * 4) == minor;
        EXPECT_TRUE(found) << "Hello firmware " << major << "." << minor;
    }
}

// Metadata header: 16 bytes, version 1.0, total size = blob size ([MS-GIPUSB] 2.2.2 example); the
// host side's own detection maps the emulated metadata back to the emulated type
TEST(XoneDeviceTables, MetadataIsWellFormed)
{
    for (auto &e : EMULATED)
    {
        SCOPED_TRACE(e.descriptor);
        Bytes md = firmware_table(SOURCE, e.descriptor);
        ASSERT_GT(md.size(), 40u);
        EXPECT_EQ(le16(md, 0), 16);
        EXPECT_EQ(le16(md, 2), 1);
        EXPECT_EQ(le16(md, 14), md.size());
        // First preferred type string
        size_t preferred = 16 + le16(md, 16 + 10);
        ASSERT_GE(md[preferred], 1);
        uint16_t len = le16(md, preferred + 1);
        EXPECT_EQ(std::string(md.begin() + preferred + 3, md.begin() + preferred + 3 + len), e.preferred_type);
        EXPECT_EQ(gip_detect_device_subtype(md.data(), md.size(), GIP_DEVICE_TYPE_MAPPINGS, GIP_DEVICE_TYPE_MAPPING_COUNT), e.detected);
    }
}

// [MS-GIPUSB] 3.1.5.6.1.2 / 3.1.5.6.1.3: the 0x20 message length in metadata MUST include the
// whole report ("Otherwise, the message is filtered out") and the report size MUST stay consistent
TEST(XoneDeviceTables, InputReportMatchesTheDeclaredLength)
{
    for (auto &e : EMULATED)
    {
        if (e.subtype == LiveGuitar)
            continue; // GhlInputReportMatchesItsMetadata
        SCOPED_TRACE(e.descriptor);
        EXPECT_EQ(declared_input_length(firmware_table(SOURCE, e.descriptor)), e.report_size);
    }
}

// The real Guitar Hero Live guitar declares message 0x20 as 14 bytes (a gamepad-style navigation
// report) and sends its 27 byte guitar state as message 0x21, declared as 32 bytes (PlasticBand
// 6-Fret Guitar/Xbox One.md and the GHL dongle metadata dump); the emulated GHL metadata copies
// that. The emulation sends its PS3-style XboxOneGHLGuitar_Data_t as 0x21, so per
// [MS-GIPUSB] 3.1.5.6.1.2 the declared 0x21 length has to cover it.
TEST(XoneDeviceTables, GhlInputReportMatchesItsMetadata)
{
    Bytes md = firmware_table(SOURCE, "xb1_descriptor_ghl");
    EXPECT_EQ(declared_input_length(md), 14u);
    EXPECT_GE(declared_input_length(md, GHL_HID_REPORT), sizeof(XboxOneGHLGuitar_Data_t));
    EXPECT_EQ(declared_input_length(md, GHL_HID_REPORT), declared_input_length(pb::GHL_DONGLE_METADATA, GHL_HID_REPORT));
}

// --- Talking to the console ---

// [MS-GIPUSB] 3.1.5.5.1 / Table 27: about half a second after enumeration the device sends Hello,
// type 0x02, flags 0x20, payload 0x1C, sequence never 0
// (queued whole by xone_device.cpp - see GipDevice.QueuedPacketsAreWhole)
TEST_F(XoneDevice, SendsHello)
{
    plug(RockBandGuitar);
    auto sent = run(400);
    EXPECT_TRUE(sent.empty());
    sent = run(400);
    ASSERT_EQ(sent.size(), 1u);
    Header h = decode(sent[0]);
    EXPECT_EQ(h.type, GIP_ANNOUNCE);
    EXPECT_EQ(h.flags, FLAG_SYSTEM);
    EXPECT_NE(h.sequence, 0);
    EXPECT_EQ(h.length, 0x1Cu);
    Bytes payload = payload_of(sent[0]);
    Bytes expected = firmware_table(SOURCE, "announce_guitar");
    ASSERT_EQ(payload.size(), expected.size());
    // The first bytes of the device ID are filled in from the clock
    EXPECT_EQ(Bytes(payload.begin() + 3, payload.end()), Bytes(expected.begin() + 3, expected.end()));
}

// The metadata exchange from the console's side: Metadata Request, then the metadata in fragments
// framed per [MS-GIPUSB] 3.1.5.2 (same sequence throughout, InitFrag + ACME on the first, ACME on
// the last data fragment, then the empty complete fragment), pausing for the console's ACKs
TEST_F(XoneDevice, SendsItsMetadata)
{
    plug(RockBandDrums);
    run(600);
    uint8_t request_seq = next_seq();
    from_console(packet(GIP_DEVICE_DESCRIPTOR, FLAG_SYSTEM, request_seq, {}));
    Bytes expected = firmware_table(SOURCE, "xb1_descriptor_drum");
    Bytes reassembled;
    bool complete = false;
    std::vector<Header> headers;
    for (int round = 0; round < 50 && !complete; round++)
    {
        for (auto &p : of_type(run(40), GIP_DEVICE_DESCRIPTOR))
        {
            Header h = decode(p);
            headers.push_back(h);
            ASSERT_TRUE(h.flags & FLAG_FRAGMENT);
            if (h.length == 0)
            {
                EXPECT_EQ(h.tlo, expected.size());
                complete = true;
                break;
            }
            EXPECT_EQ(h.tlo, (h.flags & FLAG_INIT_FRAG) ? expected.size() : reassembled.size());
            Bytes payload = payload_of(p);
            reassembled.insert(reassembled.end(), payload.begin(), payload.end());
            if (h.flags & FLAG_ACME)
                from_console(ack(GIP_DEVICE_DESCRIPTOR, h.sequence, reassembled.size(), expected.size() - reassembled.size()));
        }
    }
    ASSERT_TRUE(complete);
    EXPECT_EQ(reassembled, expected);
    ASSERT_GE(headers.size(), 3u);
    EXPECT_EQ(headers.front().flags, FLAG_FRAGMENT | FLAG_INIT_FRAG | FLAG_SYSTEM | FLAG_ACME);
    EXPECT_TRUE(headers[headers.size() - 2].flags & FLAG_ACME);
    for (auto &h : headers)
        EXPECT_EQ(h.sequence, headers.front().sequence);
}

// The device ACKs a console message that asks for one ([MS-GIPUSB] 3.1.5.1)
TEST_F(XoneDevice, AcksConsoleMessages)
{
    plug(Gamepad);
    run(600);
    from_console(packet(GIP_SET_STATE, FLAG_SYSTEM | FLAG_ACME, 0x21, {GIP_STATE_START}));
    auto acks = of_type(run(100), GIP_ACK_RESPONSE);
    ASSERT_EQ(acks.size(), 1u);
    EXPECT_EQ(acks[0], ack(GIP_SET_STATE, 0x21, 1, 0));
}

// Input reports: [MS-GIPUSB] Table 57 - type 0x20, flags 0x00, the report as payload, only sent
// when something changed
TEST_F(XoneDevice, SendsInputReportsOnChange)
{
    plug(Gamepad);
    run(600);
    start();
    mapping->bytes = {0x10}; // A
    auto reports = of_type(run(10), GIP_INPUT_REPORT);
    ASSERT_EQ(reports.size(), 1u);
    Header h = decode(reports[0]);
    EXPECT_EQ(h.flags, 0x00);
    Bytes payload = payload_of(reports[0]);
    ASSERT_EQ(payload.size(), sizeof(XboxOneGamepad_Data_t));
    EXPECT_EQ(payload[0], 0x10);
    EXPECT_TRUE(of_type(run(10), GIP_INPUT_REPORT).empty());
    mapping->bytes = {0x20}; // B
    reports = of_type(run(10), GIP_INPUT_REPORT);
    ASSERT_EQ(reports.size(), 1u);
    EXPECT_EQ(payload_of(reports[0])[0], 0x20);
}

// Guitar Hero Live: the guitar state goes out as message 0x21 (PlasticBand 6-Fret Guitar/Xbox One.md
// "Guitar Input State", 27 bytes), not as the 14 byte 0x20 navigation report
TEST_F(XoneDevice, GhlSendsItsGuitarStateAs0x21)
{
    plug(LiveGuitar);
    run(600);
    start();
    mapping->bytes = {0x01}; // White 1
    auto sent = run(10);
    EXPECT_TRUE(of_type(sent, GIP_INPUT_REPORT).empty());
    auto reports = of_type(sent, GHL_HID_REPORT);
    ASSERT_EQ(reports.size(), 1u);
    EXPECT_EQ(decode(reports[0]).flags, 0x00);
    Bytes payload = payload_of(reports[0]);
    ASSERT_EQ(payload.size(), sizeof(XboxOneGHLGuitar_Data_t));
    EXPECT_EQ(payload[0], 0x01);
}

// The GHL report is PS3 style, so byte 0 bit 1 is Black 1 (PlasticBand 6-Fret Guitar/Xbox One.md),
// not the gamepad report's guide bit: it has to reach the console instead of turning into guide
TEST_F(XoneDevice, GhlBlack1IsNotGuide)
{
    plug(LiveGuitar);
    run(600);
    start();
    mapping->bytes = {0x02}; // Black 1
    auto sent = run(10);
    EXPECT_TRUE(of_type(sent, GIP_VIRTUAL_KEYCODE).empty());
    auto reports = of_type(sent, GHL_HID_REPORT);
    ASSERT_EQ(reports.size(), 1u);
    EXPECT_EQ(payload_of(reports[0])[0], 0x02);
}

TEST_F(XoneDevice, NoInputBeforeStart)
{
    plug(Gamepad);
    mapping->bytes = {0x10};
    EXPECT_TRUE(of_type(run(1000), GIP_INPUT_REPORT).empty());
}

// [MS-GIPUSB] Table 57: "Wrapping counter ... 0x00 is reserved"; each report gets the next one
// (src/emulation/usb/xone_device.cpp: last_report_counter starts at 1 in the constructor and skips 0
// when it wraps)
TEST_F(XoneDevice, InputReportSequenceIsNeverZero)
{
    plug(Gamepad);
    run(600);
    start();
    uint8_t last = 0;
    for (int i = 0; i < 300; i++)
    {
        mapping->bytes = {(uint8_t)(i & 1 ? 0x10 : 0x20)};
        auto reports = of_type(run(2), GIP_INPUT_REPORT);
        ASSERT_EQ(reports.size(), 1u);
        uint8_t seq = reports[0][2];
        EXPECT_NE(seq, 0) << "report " << i;
        if (i)
        {
            EXPECT_EQ(seq, last == 0xFF ? 1 : last + 1);
        }
        last = seq;
    }
}

// The guide button isn't part of the 0x20 report ([MS-GIPUSB] 3.1.5.6.1.1); it goes out as Guide
// Button Status instead (GuideButtonStatus)
TEST_F(XoneDevice, GuideIsNotInTheInputReport)
{
    plug(Gamepad);
    run(600);
    start();
    mapping->bytes = {0x12}; // A + the guide bit the mappings use internally
    auto sent = run(10);
    auto reports = of_type(sent, GIP_INPUT_REPORT);
    ASSERT_FALSE(reports.empty());
    EXPECT_EQ(payload_of(reports.back())[0], 0x10);
}

// Guide Button Status: type 0x07, system message, xone gip_pkt_virtual_key {down, key} with key
// GIP_VKEY_LEFT_WIN (0x5B); xpad reads the pressed state from the first payload byte
TEST_F(XoneDevice, GuideButtonStatus)
{
    plug(Gamepad);
    run(600);
    start();
    mapping->bytes = {0x02};
    auto guide = of_type(run(100), GIP_VIRTUAL_KEYCODE);
    ASSERT_EQ(guide.size(), 1u);
    EXPECT_EQ(decode(guide[0]).flags, FLAG_SYSTEM);
    EXPECT_EQ(payload_of(guide[0]), (Bytes{0x01, 0x5B}));
    mapping->bytes = {};
    guide = of_type(run(100), GIP_VIRTUAL_KEYCODE);
    ASSERT_EQ(guide.size(), 1u);
    EXPECT_EQ(payload_of(guide[0]), (Bytes{0x00, 0x5B}));
}

// [MS-GIPUSB] Table 56: levels are percentages of the motor's power, so 100% is full rumble
TEST_F(XoneDevice, RumbleFromTheConsole)
{
    plug(Gamepad);
    from_console(packet(GIP_CMD_RUMBLE, 0x00, next_seq(), {0x00, 0x0F, 0x00, 0x00, 0x64, 0x32, 0xFF, 0x00, 0x00}));
    EXPECT_EQ(dev->rumble_calls, 1);
    EXPECT_EQ(dev->rumble_left, 255);
    EXPECT_EQ(dev->rumble_right, 127);
}

// [MS-GIPUSB] Table 56 motor bitmap: [3] left impulse, [2] right impulse, [1] left vibration,
// [0] right vibration (xpad GIP_MOTOR_L = BIT(1), GIP_MOTOR_R = BIT(0)). A console driving only
// the main motors sends 0x03. src/emulation/usb/xone_device.cpp (interrupt_xfer GIP_CMD_RUMBLE)
// applies the left / right vibration levels when the vibration motor bits (0x02 / 0x01) are set,
// not the impulse trigger bits (0x08 / 0x04).
TEST_F(XoneDevice, RumbleUsesTheVibrationMotorBits)
{
    plug(Gamepad);
    from_console(packet(GIP_CMD_RUMBLE, 0x00, next_seq(), {0x00, 0x03, 0x00, 0x00, 0x64, 0x32, 0xFF, 0x00, 0x00}));
    EXPECT_EQ(dev->rumble_left, 255);
    EXPECT_EQ(dev->rumble_right, 127);
    // Only the impulse triggers: the vibration levels mustn't be applied
    from_console(packet(GIP_CMD_RUMBLE, 0x00, next_seq(), {0x00, 0x0C, 0x64, 0x64, 0x64, 0x64, 0xFF, 0x00, 0x00}));
    EXPECT_EQ(dev->rumble_left, 0);
    EXPECT_EQ(dev->rumble_right, 0);
}

// [MS-GIPUSB] Table 41: LED Guide Button - payload 0x00, pattern, intensity
TEST_F(XoneDevice, GuideLedFromTheConsole)
{
    plug(Gamepad);
    from_console(packet(GIP_CMD_LED_ON, FLAG_SYSTEM, next_seq(), {0x00, 0x01, 0x14}));
    EXPECT_EQ(dev->player_led_calls, 1);
    EXPECT_EQ(dev->player_led, 1);
    from_console(packet(GIP_CMD_LED_ON, FLAG_SYSTEM, next_seq(), {0x00, 0x00, 0x00}));
    EXPECT_EQ(dev->player_led, 0);
}

// Security messages from the console are relayed whole, once reassembled
TEST_F(XoneDevice, RelaysConsoleSecurityMessages)
{
    std::vector<Bytes> relayed;
    auth_broker.register_handler(ModeXboxOne, [&](XGIPProtocol *p) {
        relayed.emplace_back(p->getData(), p->getData() + p->getDataLength());
    });
    plug(Gamepad);
    Bytes message = message_bytes(218, 3);
    for (auto &f : fragments(GIP_AUTH, 0x05, message))
        from_console(f);
    EXPECT_TRUE(relayed.empty());
    from_console(fragment_complete(GIP_AUTH, 0x05, message.size()));
    ASSERT_EQ(relayed.size(), 1u);
    EXPECT_EQ(relayed[0], message);
    auth_broker.unregister_handler(ModeXboxOne);
}

// [MS-GIPUSB] 3.1.5.5.5 Set Device State RESET: input stops and the device goes back to sending
// Hello (only that something is sent is checked here; see SendsHello)
TEST_F(XoneDevice, ResetGoesBackToHello)
{
    plug(Gamepad);
    run(600);
    start();
    mapping->bytes = {0x10};
    EXPECT_FALSE(of_type(run(10), GIP_INPUT_REPORT).empty());
    from_console(packet(GIP_SET_STATE, FLAG_SYSTEM, next_seq(), {GIP_STATE_RESET}));
    mapping->bytes = {0x20};
    auto sent = run(10);
    EXPECT_TRUE(of_type(sent, GIP_INPUT_REPORT).empty());
    sent = run(600);
    EXPECT_EQ(sent.size(), 1u);
    EXPECT_TRUE(of_type(sent, GIP_INPUT_REPORT).empty());
}

// With a real controller plugged into the USB host to answer the console's security challenge,
// START doesn't make the device ready: input waits for the console's security "auth done"
// message (type 0x06, payload 01 00 - the same as xpad's xboxone_auth_done), which is still
// relayed to the controller
TEST_F(XoneDevice, InputWaitsForTheConsoleToFinishAuth)
{
    std::vector<Bytes> relayed;
    auth_broker.register_handler(ModeXboxOne, [&](XGIPProtocol *p) {
        relayed.emplace_back(p->getData(), p->getData() + p->getDataLength());
    });
    plug(Gamepad);
    run(600);
    start();
    mapping->bytes = {0x10};
    EXPECT_TRUE(of_type(run(100), GIP_INPUT_REPORT).empty());
    EXPECT_FALSE(auth_broker.is_auth_completed(ModeXboxOne));

    from_console(packet(GIP_AUTH, FLAG_SYSTEM, next_seq(), {0x01, 0x00}));
    EXPECT_TRUE(auth_broker.is_auth_completed(ModeXboxOne));
    ASSERT_EQ(relayed.size(), 1u);
    EXPECT_EQ(relayed[0], (Bytes{0x01, 0x00}));
    EXPECT_FALSE(of_type(run(10), GIP_INPUT_REPORT).empty());
    auth_broker.unregister_handler(ModeXboxOne);
}

// [MS-GIPUSB] 3.1.5.5.2.2 / Table 30: a USB powered device reports status 0x80 (full power, no
// battery) in a 4 byte Extended Status message, type 0x03, flags 0x20, and keeps sending it
// periodically
TEST_F(XoneDevice, SendsStatusPeriodically)
{
    plug(Gamepad);
    run(600);
    start();
    auto status = of_type(run(XBONE_KEEPALIVE_TIMER + 100), GIP_STATUS);
    ASSERT_GE(status.size(), 1u);
    Header h = decode(status[0]);
    EXPECT_EQ(h.flags, FLAG_SYSTEM);
    EXPECT_NE(h.sequence, 0);
    EXPECT_EQ(payload_of(status[0]), (Bytes{0x80, 0x00, 0x00, 0x00}));
}
