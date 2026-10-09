// XGIPProtocol (lib/xgip_protocol): single packet GIP headers, parsing, ACKs and sequence numbers.
// Expected bytes come from [MS-GIPUSB] 2.2.10 (message header) and 3.1.5.4 (Table 26), the
// init packets in Linux xpad.c and the ACK layout in xone's bus/protocol.c.
#include <gtest/gtest.h>

#include "gip_test_support.hpp"
#include "protocols/xbox_one.hpp"
#include "xgip_protocol.h"

using namespace gip_test;

namespace
{
Bytes generated(XGIPProtocol &p)
{
    uint8_t *data = p.generatePacket();
    return Bytes(data, data + p.getPacketLength());
}

Bytes generated_ack(XGIPProtocol &p)
{
    uint8_t *data = p.generateAckPacket();
    return Bytes(data, data + p.getPacketLength());
}
} // namespace

// [MS-GIPUSB] Table 14: client / expansion index in bits 0-3, ACME bit 4, System bit 5,
// InitFrag bit 6, Fragment bit 7
TEST(GipHeader, BitfieldMatchesTheFlagsByte)
{
    static_assert(sizeof(GipHeader_t) == 4, "GIP single packet header is 4 bytes ([MS-GIPUSB] Table 12)");
    GipHeader_t h = {};
    h.needsAck = 1;
    EXPECT_EQ(((uint8_t *)&h)[1], FLAG_ACME);
    h = {};
    h.internal = 1;
    EXPECT_EQ(((uint8_t *)&h)[1], FLAG_SYSTEM);
    h = {};
    h.chunkStart = 1;
    EXPECT_EQ(((uint8_t *)&h)[1], FLAG_INIT_FRAG);
    h = {};
    h.chunked = 1;
    EXPECT_EQ(((uint8_t *)&h)[1], FLAG_FRAGMENT);
    h = {};
    h.client = 0x7;
    EXPECT_EQ(((uint8_t *)&h)[1], 0x07);
}

// xpad xboxone_power_on: GIP_CMD_POWER, GIP_OPT_INTERNAL, seq, GIP_PL_LEN(1), GIP_PWR_ON
// ([MS-GIPUSB] Table 26: Set Device State, flags 0x20, payload length 0x01)
TEST(GipHeader, SystemMessageEncodes)
{
    XGIPProtocol p;
    p.setAttributes(GIP_SET_STATE, 0x07, 1, 0, 0);
    const uint8_t start = 0x00;
    p.setData(&start, 1);
    EXPECT_EQ(generated(p), (Bytes{0x05, 0x20, 0x07, 0x01, 0x00}));
    EXPECT_FALSE(p.waitingToSend());
}

// xpad xboxone_rumblebegin_init: GIP_CMD_RUMBLE with no option bits, 9 byte payload
// ([MS-GIPUSB] Table 56: Direct Motor Command flags 0x00)
TEST(GipHeader, VendorMessageHasNoSystemFlag)
{
    XGIPProtocol p;
    p.setAttributes(GIP_CMD_RUMBLE, 0x01, 0, 0, 0);
    const Bytes rumble = {0x00, 0x0F, 0x00, 0x00, 0x1D, 0x1D, 0xFF, 0x00, 0x00};
    p.setData(rumble.data(), rumble.size());
    EXPECT_EQ(generated(p), concat({{0x09, 0x00, 0x01, 0x09}, rumble}));
}

// [MS-GIPUSB] Table 26: Metadata Request is type 0x04, flags 0x20, payload length 0
// (xone gip_request_identification sends the same with no payload)
TEST(GipHeader, EmptyPayload)
{
    XGIPProtocol p;
    p.setAttributes(GIP_DEVICE_DESCRIPTOR, 0x03, 1, 0, 0);
    EXPECT_EQ(generated(p), (Bytes{0x04, 0x20, 0x03, 0x00}));
}

// [MS-GIPUSB] Table 14: ACME (bit 4) requests an acknowledgement
TEST(GipHeader, AckRequestSetsAcme)
{
    XGIPProtocol p;
    p.setAttributes(GIP_VIRTUAL_KEYCODE, 0x10, 1, 0, 1);
    const Bytes key = {0x01, 0x5B};
    p.setData(key.data(), key.size());
    EXPECT_EQ(generated(p), (Bytes{0x07, 0x30, 0x10, 0x02, 0x01, 0x5B}));
}

TEST(GipParse, SinglePacket)
{
    // [MS-GIPUSB] Table 57: gamepad input report, type 0x20, flags 0x00, 14 byte payload
    Bytes report = message_bytes(14);
    Bytes wire = packet(GIP_INPUT_REPORT, 0x00, 0x05, report);
    XGIPProtocol p;
    ASSERT_TRUE(p.parse(wire.data(), wire.size()));
    EXPECT_TRUE(p.validate());
    EXPECT_EQ(p.getCommand(), GIP_INPUT_REPORT);
    EXPECT_EQ(p.getSequence(), 0x05);
    EXPECT_EQ(p.getChunked(), 0);
    EXPECT_FALSE(p.ackRequired());
    EXPECT_EQ(p.getParsedWireLength(), 18);
    ASSERT_EQ(p.getDataLength(), 14);
    EXPECT_EQ(Bytes(p.getData(), p.getData() + 14), report);
}

TEST(GipParse, ShorterThanAHeaderIsRejected)
{
    const Bytes wire = {0x20, 0x00, 0x01};
    XGIPProtocol p;
    EXPECT_FALSE(p.parse(wire.data(), wire.size()));
    EXPECT_FALSE(p.validate());
    EXPECT_EQ(p.getParsedWireLength(), 0);
}

TEST(GipParse, TruncatedPayloadIsRejected)
{
    Bytes wire = packet(GIP_INPUT_REPORT, 0x00, 0x05, message_bytes(14));
    wire.resize(10);
    XGIPProtocol p;
    EXPECT_FALSE(p.parse(wire.data(), wire.size()));
    EXPECT_FALSE(p.validate());
}

// [MS-GIPUSB] 3.1.5.3: several small messages can be packed into one transfer; the parsed wire
// length is how far to step to the next one
TEST(GipParse, CoalescedMessagesReportTheirOwnLength)
{
    Bytes first = packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM, 0x01, {0x01, 0x5B});
    Bytes second = packet(GIP_INPUT_REPORT, 0x00, 0x02, message_bytes(14));
    Bytes wire = concat({first, second});
    XGIPProtocol p;
    ASSERT_TRUE(p.parse(wire.data(), wire.size()));
    EXPECT_EQ(p.getCommand(), GIP_VIRTUAL_KEYCODE);
    EXPECT_EQ(p.getParsedWireLength(), first.size());
    ASSERT_TRUE(p.parse(wire.data() + first.size(), wire.size() - first.size()));
    EXPECT_EQ(p.getCommand(), GIP_INPUT_REPORT);
    EXPECT_EQ(p.getParsedWireLength(), second.size());
}

// xone gip_pkt_acknowledge is 9 bytes and xone / xpad always send ACKs as system messages
TEST(GipParse, AckMessages)
{
    XGIPProtocol p;
    Bytes good = ack(GIP_DEVICE_DESCRIPTOR, 0x04, 58, 124);
    ASSERT_TRUE(p.parse(good.data(), good.size()));
    EXPECT_EQ(p.getCommand(), GIP_ACK_RESPONSE);
    EXPECT_EQ(p.getSequence(), 0x04);
    EXPECT_EQ(p.getParsedWireLength(), 13);

    Bytes short_payload = {0x01, 0x20, 0x04, 0x08, 0x00, 0x04, 0x20, 0x3A, 0x00, 0x00, 0x00, 0x7C};
    EXPECT_FALSE(p.parse(short_payload.data(), short_payload.size()));
    Bytes not_system = good;
    not_system[1] = 0x00;
    EXPECT_FALSE(p.parse(not_system.data(), not_system.size()));
}

// xpad xpadone_ack_mode_report: the Xbox One S pad's guide report (sent with ACME) is acked
// with GIP_CMD_ACK, GIP_OPT_INTERNAL, <seq>, 9, 0x00, GIP_CMD_VIRTUAL_KEY, GIP_OPT_INTERNAL, 0x02,
// 0x00, 0x00, 0x00, 0x00, 0x00 - two bytes received, nothing remaining
TEST(GipAck, SinglePacketAckMatchesXpad)
{
    const Bytes guide = {0x07, 0x30, 0x2A, 0x02, 0x01, 0x5B};
    XGIPProtocol p;
    ASSERT_TRUE(p.parse(guide.data(), guide.size()));
    EXPECT_TRUE(p.ackRequired());
    EXPECT_EQ(generated_ack(p), (Bytes{0x01, 0x20, 0x2A, 0x09, 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}));
}

TEST(GipAck, NoAckWithoutAcme)
{
    const Bytes guide = {0x07, 0x20, 0x2A, 0x02, 0x01, 0x5B};
    XGIPProtocol p;
    ASSERT_TRUE(p.parse(guide.data(), guide.size()));
    EXPECT_FALSE(p.ackRequired());
}

// [MS-GIPUSB] 2.2.10: "0x00 is reserved for both system and vendor messages", so a wrapping
// sequence counter goes 0xFF -> 0x01
TEST(GipSequence, IncrementSkipsZero)
{
    XGIPProtocol p;
    p.setSequence(0xFE);
    p.incrementSequence();
    EXPECT_EQ(p.getSequence(), 0xFF);
    p.incrementSequence();
    EXPECT_EQ(p.getSequence(), 0x01);
}

// copyAttributes is how auth messages are relayed between the console and controller
TEST(GipParse, CopyAttributesKeepsTheMessage)
{
    Bytes wire = packet(GIP_AUTH, FLAG_SYSTEM | FLAG_ACME, 0x09, message_bytes(20));
    XGIPProtocol in;
    ASSERT_TRUE(in.parse(wire.data(), wire.size()));
    XGIPProtocol out;
    out.copyAttributes(&in);
    EXPECT_EQ(out.getCommand(), GIP_AUTH);
    EXPECT_EQ(out.getSequence(), 0x09);
    EXPECT_EQ(generated(out), wire);
}
