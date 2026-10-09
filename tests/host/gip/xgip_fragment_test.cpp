// XGIPProtocol (lib/xgip_protocol): reliable large message transmission. Messages over 58 bytes
// are split into fragments ([MS-GIPUSB] 3.1.5.2, Tables 21-25), acknowledged per 3.1.5.1, and
// finished with an empty "complete" fragment (Table 26 "Metadata Complete"). Expected framing
// follows the spec tables and xone's bus/protocol.c (gip_send_pkt, gip_send_remaining_chunks,
// gip_acknowledge_pkt, gip_process_pkt_chunked).
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

// Every packet XGIPProtocol sends for a message, the way the firmware drives it: keep calling
// generatePacket() while waitingToSend() (the ACKs the sender waits on don't change the framing)
std::vector<Bytes> send_all(uint8_t type, uint8_t sequence, const Bytes &message)
{
    XGIPProtocol p;
    p.setAttributes(type, sequence, 1, 1, 0);
    p.setData(message.data(), message.size());
    std::vector<Bytes> out;
    do
    {
        out.push_back(generated(p));
    } while (p.waitingToSend() && out.size() < 64);
    return out;
}
} // namespace

// [MS-GIPUSB] Table 22 (first fragment, total >= 128): Fragment | InitFrag | System | ACME = 0xF0,
// a one byte payload length of 58 and the total length as a two byte TLO.
TEST(GipFragmentSend, FirstFragmentFormat2)
{
    Bytes message = message_bytes(218);
    auto packets = send_all(GIP_DEVICE_DESCRIPTOR, 0x05, message);
    ASSERT_GE(packets.size(), 1u);
    Bytes expected = concat({{0x04, 0xF0, 0x05, 0x3A, 0xDA, 0x01}, Bytes(message.begin(), message.begin() + 58)});
    EXPECT_EQ(packets[0], expected);
}

// [MS-GIPUSB] Table 21 (first fragment, total < 128): the payload length is padded to two bytes
// (0x80 | 58, 0x00) so the header stays 6 bytes, then a one byte total length
TEST(GipFragmentSend, FirstFragmentFormat1)
{
    Bytes message = message_bytes(100);
    auto packets = send_all(GIP_AUTH, 0x02, message);
    ASSERT_GE(packets.size(), 1u);
    Bytes expected = concat({{0x06, 0xF0, 0x02, 0xBA, 0x00, 0x64}, Bytes(message.begin(), message.begin() + 58)});
    EXPECT_EQ(packets[0], expected);
}

// The whole 218 byte transfer, fragment for fragment, against the spec framing: Table 22 first,
// Table 23 middles (offset < 128, length padded), Table 25 last with ACME (offset >= 128 so the
// offset is the two byte field), then the empty complete fragment with the total as TLO. This is
// also exactly what xone produces (gip_send_pkt / gip_send_remaining_chunks /
// gip_handle_pkt_acknowledge), which test support's fragments() mirrors.
TEST(GipFragmentSend, WholeTransferMatchesSpecFraming)
{
    Bytes message = message_bytes(218);
    auto packets = send_all(GIP_DEVICE_DESCRIPTOR, 0x05, message);
    auto expected = fragments(GIP_DEVICE_DESCRIPTOR, 0x05, message);
    expected.push_back(fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x05, 218));
    ASSERT_EQ(packets.size(), expected.size());
    for (size_t i = 0; i < packets.size(); i++)
        EXPECT_EQ(packets[i], expected[i]) << "packet " << i;
}

// [MS-GIPUSB] 3.1.5.2: all fragments share the sequence; the first and last data fragments have
// ACME, a fragment requests ACME at least every 5 ("usually just every fourth or fifth packet"),
// and offsets run contiguously so the message reassembles
TEST(GipFragmentSend, LongTransferStructure)
{
    Bytes message = message_bytes(700, 3);
    auto packets = send_all(GIP_AUTH, 0x21, message);
    ASSERT_GE(packets.size(), 3u);
    Bytes reassembled;
    size_t since_ack = 0;
    for (size_t i = 0; i < packets.size(); i++)
    {
        Header h = decode(packets[i]);
        SCOPED_TRACE(i);
        EXPECT_EQ(h.type, GIP_AUTH);
        EXPECT_EQ(h.sequence, 0x21);
        EXPECT_TRUE(h.flags & FLAG_FRAGMENT);
        EXPECT_TRUE(h.flags & FLAG_SYSTEM);
        EXPECT_EQ((bool)(h.flags & FLAG_INIT_FRAG), i == 0);
        bool complete = i == packets.size() - 1;
        if (complete)
        {
            EXPECT_EQ(h.length, 0u);
            EXPECT_EQ(h.tlo, message.size());
            EXPECT_FALSE(h.flags & FLAG_ACME);
            continue;
        }
        bool last_data = i == packets.size() - 2;
        if (i == 0)
        {
            EXPECT_EQ(h.tlo, message.size());
        }
        else
        {
            EXPECT_EQ(h.tlo, reassembled.size());
        }
        if (i == 0 || last_data)
        {
            EXPECT_TRUE(h.flags & FLAG_ACME);
        }
        since_ack = (h.flags & FLAG_ACME) ? 0 : since_ack + 1;
        EXPECT_LT(since_ack, 5u);
        EXPECT_LE(h.length, FRAGMENT_PAYLOAD);
        Bytes payload(packets[i].begin() + h.header_size, packets[i].end());
        EXPECT_EQ(payload.size(), h.length);
        reassembled.insert(reassembled.end(), payload.begin(), payload.end());
    }
    EXPECT_EQ(reassembled, message);
}

// [MS-GIPUSB] 3.1.5.2: "Downstream GIP headers are required to have an even number of bytes due
// to specific device constraints", and Table 25 gives the last fragment a two byte offset field
// for exactly this. XGIPProtocol is used downstream when the USB host forwards the console's
// security messages to a real controller (XboxOneHost::send_report_from_host). So
// lib/xgip_protocol/xgip_protocol.cpp generatePacket pads a one byte offset on the last data
// fragment (Table 25, e.g. 100 bytes -> 06 B0 02 2A BA 00), and writeLeb128 pads any value up to
// 0x7F (the complete fragment of a 127 byte message is 06 A0 02 00 FF 00).
TEST(GipFragmentSend, EveryHeaderIsEvenLength)
{
    for (size_t size = 59; size <= 300; size++)
    {
        auto packets = send_all(GIP_AUTH, 0x02, message_bytes(size));
        for (size_t i = 0; i < packets.size(); i++)
        {
            EXPECT_EQ(decode(packets[i]).header_size % 2, 0u) << size << " byte message, packet " << i;
        }
    }
}

// A message that fits in one packet isn't fragmented (xone gip_send_pkt: "packet fits into single
// buffer"); the firmware comment says it then goes out as a single packet that still requests an
// ACK. A single packet header is type, flags, sequence, length ([MS-GIPUSB] Table 12) - there's
// no TLO field without the Fragment flag. (lib/xgip_protocol/xgip_protocol.cpp generatePacket sends
// it through the single packet path, once, rather than through the fragment encoder.)
TEST(GipFragmentSend, ShortReliableMessageIsASinglePacket)
{
    Bytes message = message_bytes(10);
    auto packets = send_all(GIP_AUTH, 0x02, message);
    ASSERT_EQ(packets.size(), 1u);
    EXPECT_EQ(packets[0], concat({{0x06, 0x30, 0x02, 0x0A}, message}));
}

// The short reliable message decodes (per the spec header layout) as an unfragmented message with
// ACME and the whole message as its payload
TEST(GipFragmentSend, ShortReliableMessageFirstPacketPayload)
{
    Bytes message = message_bytes(10);
    auto packets = send_all(GIP_AUTH, 0x02, message);
    ASSERT_GE(packets.size(), 1u);
    Header h = decode(packets[0]);
    EXPECT_FALSE(h.flags & FLAG_FRAGMENT);
    EXPECT_TRUE(h.flags & FLAG_ACME);
    EXPECT_EQ(h.length, message.size());
}

// --- Receiving ---

// Reassembly of a device's 218 byte metadata the way xone sends it, with the ACKs xone's
// gip_acknowledge_pkt would send back: bytes received through the acked fragment and the bytes
// still remaining ([MS-GIPUSB] 3.1.5.1: the receiver always ACKs fragments with ACME)
TEST(GipFragmentReceive, ReassemblesAndAcks)
{
    Bytes message = message_bytes(218, 9);
    auto frags = fragments(GIP_DEVICE_DESCRIPTOR, 0x11, message);
    ASSERT_EQ(frags.size(), 4u);
    XGIPProtocol p;
    size_t received = 0;
    for (size_t i = 0; i < frags.size(); i++)
    {
        SCOPED_TRACE(i);
        ASSERT_TRUE(p.parse(frags[i].data(), frags[i].size()));
        EXPECT_EQ(p.getParsedWireLength(), frags[i].size());
        EXPECT_TRUE(p.getChunked());
        EXPECT_FALSE(p.endOfChunk());
        received += decode(frags[i]).length;
        bool acme = decode(frags[i]).flags & FLAG_ACME;
        EXPECT_EQ(p.ackRequired(), acme);
        if (acme)
        {
            EXPECT_EQ(generated_ack(p), ack(GIP_DEVICE_DESCRIPTOR, 0x11, received, 218 - received));
        }
    }
    Bytes done = fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x11, 218);
    ASSERT_TRUE(p.parse(done.data(), done.size()));
    EXPECT_TRUE(p.endOfChunk());
    EXPECT_FALSE(p.ackRequired());
    EXPECT_EQ(p.getParsedWireLength(), done.size());
    EXPECT_EQ(p.getCommand(), GIP_DEVICE_DESCRIPTOR);
    ASSERT_EQ(p.getDataLength(), 218);
    EXPECT_EQ(Bytes(p.getData(), p.getData() + 218), message);
}

// Tables 21 / 23 (padded payload length, one byte TLO) for a message under 128 bytes
TEST(GipFragmentReceive, ShortMessageFormat1)
{
    Bytes message = message_bytes(100, 1);
    auto frags = fragments(GIP_AUTH, 0x03, message);
    XGIPProtocol p;
    for (auto &f : frags)
        ASSERT_TRUE(p.parse(f.data(), f.size()));
    EXPECT_EQ(generated_ack(p), ack(GIP_AUTH, 0x03, 100, 0));
    Bytes done = fragment_complete(GIP_AUTH, 0x03, 100);
    ASSERT_TRUE(p.parse(done.data(), done.size()));
    EXPECT_TRUE(p.endOfChunk());
    EXPECT_EQ(Bytes(p.getData(), p.getData() + p.getDataLength()), message);
}

// Table 25 pads the offset field instead of the length ("1XXX_XXXX 0XXX_XXXX"); "accessories need
// to support both formats downstream because the host uses both"
TEST(GipFragmentReceive, AcceptsPaddedOffsetField)
{
    Bytes message = message_bytes(100, 2);
    XGIPProtocol p;
    Bytes first = concat({{0x06, 0xF0, 0x04, 0xBA, 0x00, 0x64}, Bytes(message.begin(), message.begin() + 58)});
    Bytes last = concat({{0x06, 0xB0, 0x04, 0x2A, 0xBA, 0x00}, Bytes(message.begin() + 58, message.end())});
    Bytes done = {0x06, 0xA0, 0x04, 0x00, 0xE4, 0x00};
    ASSERT_TRUE(p.parse(first.data(), first.size()));
    ASSERT_TRUE(p.parse(last.data(), last.size()));
    EXPECT_EQ(p.getParsedWireLength(), last.size());
    ASSERT_TRUE(p.parse(done.data(), done.size()));
    EXPECT_TRUE(p.endOfChunk());
    EXPECT_EQ(Bytes(p.getData(), p.getData() + p.getDataLength()), message);
}

// The receiver must not accept a completion whose total doesn't match the transfer
TEST(GipFragmentReceive, CompletionWithWrongTotalIsRejected)
{
    Bytes message = message_bytes(218);
    XGIPProtocol p;
    for (auto &f : fragments(GIP_DEVICE_DESCRIPTOR, 0x01, message))
        ASSERT_TRUE(p.parse(f.data(), f.size()));
    Bytes done = fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x01, 200);
    EXPECT_FALSE(p.parse(done.data(), done.size()));
    EXPECT_FALSE(p.endOfChunk());
}

// A fragment that would write past the 1024 byte reassembly buffer is dropped, not copied
TEST(GipFragmentReceive, FragmentPastTheBufferIsRejected)
{
    XGIPProtocol p;
    Bytes first = concat({header(GIP_AUTH, 0xF0, 0x01, 58, true, 2000), message_bytes(58)});
    ASSERT_TRUE(p.parse(first.data(), first.size()));
    Bytes far = concat({header(GIP_AUTH, 0xA0, 0x01, 58, true, 1000), message_bytes(58)});
    EXPECT_FALSE(p.parse(far.data(), far.size()));
}

// A fragment whose payload runs past the end of the transfer is rejected
TEST(GipFragmentReceive, TruncatedFragmentIsRejected)
{
    XGIPProtocol p;
    Bytes first = concat({header(GIP_AUTH, 0xF0, 0x01, 58, true, 218), message_bytes(58)});
    first.resize(40);
    EXPECT_FALSE(p.parse(first.data(), first.size()));
}

// Whatever XGIPProtocol sends, XGIPProtocol reads back: the emulated device's metadata goes
// through the same code on the host side
TEST(GipFragmentReceive, RoundTripsEverySize)
{
    for (size_t size = 59; size <= 1024; size += 7)
    {
        Bytes message = message_bytes(size, (uint8_t)size);
        auto packets = send_all(GIP_DEVICE_DESCRIPTOR, 0x01, message);
        XGIPProtocol rx;
        for (auto &pkt : packets)
            ASSERT_TRUE(rx.parse(pkt.data(), pkt.size())) << size;
        ASSERT_TRUE(rx.endOfChunk()) << size;
        EXPECT_EQ(Bytes(rx.getData(), rx.getData() + rx.getDataLength()), message) << size;
    }
}

// [MS-GIPUSB] 2.2.10.4: extension bytes extend "to a maximum of 4 bytes", and a parser must not
// read a header field past the bytes it was given (xone gip_decode_varint stops at `len`). Here the
// 4 byte transfer ends on a continuation byte; lib/xgip_protocol/xgip_protocol.cpp readLeb128
// stops at the buffer end and parse() rejects the packet instead of reading the bytes after it
// (which belong to whatever follows in memory) as the payload length / TLO.
TEST(GipFragmentReceive, HeaderFieldsStopAtTheBufferEnd)
{
    // Only the first 4 bytes are the transfer; the zeros after it are not
    const uint8_t memory[] = {0x04, 0xE0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00};
    XGIPProtocol p;
    bool ok = p.parse(memory, 4);
    EXPECT_FALSE(ok && p.getParsedWireLength() > 4) << "parsed wire length " << p.getParsedWireLength();
    EXPECT_FALSE(p.endOfChunk());
}

// A varint longer than the 4 bytes 2.2.10.4 allows is rejected by lib/xgip_protocol/xgip_protocol.cpp
// readLeb128 rather than shifted further (a shift of 35 would be undefined for the int it shifts;
// a malformed packet from a USB device is enough to hit it). Run in a child so a UBSan abort shows
// up as this test failing rather than taking the run down.
TEST(GipFragmentReceive, LongVarintIsNotUndefined)
{
    static const uint8_t wire[] = {0x04, 0xA0, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00};
    auto parse_and_exit = [] {
        XGIPProtocol p;
        p.parse(wire, sizeof(wire));
        std::exit(0);
    };
    EXPECT_EXIT(parse_and_exit(), ::testing::ExitedWithCode(0), "");
}
