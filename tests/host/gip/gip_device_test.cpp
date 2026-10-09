// lib/gip_common/gip_device.cpp + gip_packet_handler.cpp: the USB host's side of a GIP
// conversation with a controller - Hello / metadata / auth, ACKs, reliable transfers, status,
// guide button and the init packets it sends. Played against a recording gip_device_interface_t.
#include <gtest/gtest.h>

#include "gip_button_mapping.h"
#include "gip_device.h"
#include "gip_fakes.hpp"
#include "gip_packet_handler.h"
#include "gip_report_queue.h"
#include "gip_test_support.hpp"
#include "managers/config_manager.hpp"
#include "plasticband_metadata.hpp"
#include "usb/auth_broker.h"
#include "xgip_protocol.h"

using namespace gip_test;
namespace pb = gip_test::plasticband;

namespace
{
class GipDevice : public ::testing::Test
{
protected:
    gip_device_t dev;
    Recorder rec;

    void SetUp() override
    {
        gip_fake::reset();
        gip_device_init(&dev);
        dev.user_context = &rec;
        dev.interface = &Recorder::interface;
        rec.device = &dev;
        rec.request_descriptor_on_arrival = true;
    }
    void TearDown() override
    {
        gip_device_cleanup(&dev);
        gip_fake::reset();
    }

    bool feed(const Bytes &wire)
    {
        return gip_device_process_incoming(&dev, wire.data(), (uint16_t)wire.size());
    }
    void update(uint32_t now)
    {
        gip_device_update(&dev, now, XGIP_ACK_WAIT_TIMEOUT, Recorder::queue_packet, &rec);
    }
    bool digital(uint8_t subtype, proto_Output out)
    {
        return gip_tick_digital(dev.raw_input, subtype, dev.capture, &out);
    }
};

proto_Output gamepad_button(proto_GamepadButtonType b)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_gamepadButton_tag;
    o.mapping.gamepadButton = b;
    return o;
}

proto_Output ghl_button(proto_GuitarHeroLiveGuitarButtonType b)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_ghlButton_tag;
    o.mapping.ghlButton = b;
    return o;
}
} // namespace

// [MS-GIPUSB] 3.1.5.5.1 / Figure 3: the host answers a Hello with a Metadata Request, type 0x04,
// flags 0x20, no payload (Table 26; xone gip_request_identification). The sequence comes from
// the global pool (Table 15) and is never 0.
TEST_F(GipDevice, HelloRequestsMetadata)
{
    EXPECT_TRUE(feed(packet(GIP_ANNOUNCE, FLAG_SYSTEM, 0x01, pb::MADCATZ_STRATOCASTER_HELLO)));
    EXPECT_EQ(rec.arrivals, 1);
    ASSERT_EQ(rec.queued.size(), 1u);
    EXPECT_EQ(rec.queued[0], (Bytes{0x04, 0x20, 0x01, 0x00}));
}

// Every packet gip_packet_handler.cpp queues must be handed over with its own length.
// (lib/gip_common/gip_packet_handler.cpp: gip_send_power_on_sequence, gip_request_device_descriptor,
// gip_send_auth_complete, gip_send_ghl_poke; likewise xone_host.cpp send_report_from_host /
// send_feedback_packet and xone_device.cpp.) The order function arguments are evaluated in is
// unspecified and getPacketLength() is only right after generatePacket() has run, so
// `queue(ctx, xgip->generatePacket(), xgip->getPacketLength())` isn't safe (host GCC evaluates the
// length first): the packet pointer is stored in a local before the length is read.
TEST_F(GipDevice, QueuedPacketsAreWhole)
{
    dev.subtype = Gamepad;
    gip_send_power_on_sequence(&dev);
    gip_request_device_descriptor(&dev);
    gip_send_auth_complete(&dev);
    gip_send_ghl_poke(&dev);
    ASSERT_EQ(rec.queued.size(), 6u);
    for (size_t i = 0; i < rec.queued.size(); i++)
        EXPECT_EQ(rec.queued_lengths[i], rec.queued[i].size()) << "packet " << i << " type " << (int)rec.queued[i][0];
}

// [MS-GIPUSB] 3.1.5.3: several messages in one transfer are all processed. Bytes past the last
// message that are zero are padding (0x00 isn't a GIP message type the host handles).
TEST_F(GipDevice, CoalescedMessages)
{
    Bytes report = message_bytes(14);
    Bytes wire = concat({packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM, 0x05, {0x01, 0x5B}),
                         packet(GIP_INPUT_REPORT, 0x00, 0x06, report), Bytes(8, 0)});
    EXPECT_TRUE(feed(wire));
    EXPECT_TRUE(dev.has_virtual_key_guide);
    // The report is stored as is, plus the guide state from the 0x07 message before it
    EXPECT_EQ(dev.raw_input[0], report[0] | 0x02);
    EXPECT_EQ(Bytes(dev.raw_input + 1, dev.raw_input + 14), Bytes(report.begin() + 1, report.end()));
}

// [MS-GIPUSB] 3.1.5.1: a message with ACME is acked with Protocol Control; the ACK goes out ahead
// of anything else (send_ack). Layout per xone gip_acknowledge_pkt / xpad.
TEST_F(GipDevice, AcmeMessageIsAcked)
{
    feed(packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM | FLAG_ACME, 0x33, {0x01, 0x5B}));
    ASSERT_EQ(rec.acks.size(), 1u);
    EXPECT_EQ(rec.acks[0], ack(GIP_VIRTUAL_KEYCODE, 0x33, 2, 0));
}

TEST_F(GipDevice, NoAckWithoutAcme)
{
    feed(packet(GIP_INPUT_REPORT, 0x00, 0x33, message_bytes(14)));
    EXPECT_TRUE(rec.acks.empty());
}

// The whole metadata exchange with a real Stratocaster's metadata (PlasticBand dump), sent the
// way xone sends fragmented messages: ACKs for the first and last fragments with the bytes
// received / remaining, detection on the complete fragment, then the auth-done packet xpad sends
// to start input (xboxone_auth_done: GIP_CMD_AUTHENTICATE, GIP_OPT_INTERNAL, seq, 2, 0x01, 0x00).
// Security messages have their own sequence pool ([MS-GIPUSB] Table 15), so it's 1.
TEST_F(GipDevice, MetadataTransferDetectsTheDeviceAndCompletesAuth)
{
    const Bytes &md = pb::MADCATZ_STRATOCASTER_METADATA;
    auto frags = fragments(GIP_DEVICE_DESCRIPTOR, 0x02, md);
    for (auto &f : frags)
        EXPECT_TRUE(feed(f));
    ASSERT_EQ(rec.acks.size(), 2u);
    EXPECT_EQ(rec.acks[0], ack(GIP_DEVICE_DESCRIPTOR, 0x02, 58, md.size() - 58));
    EXPECT_EQ(rec.acks[1], ack(GIP_DEVICE_DESCRIPTOR, 0x02, md.size(), 0));
    EXPECT_TRUE(rec.descriptors.empty()) << "detected before the transfer completed";
    EXPECT_TRUE(dev.incoming_chunk_pending);

    feed(fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x02, md.size()));
    EXPECT_FALSE(dev.incoming_chunk_pending);
    ASSERT_EQ(rec.descriptors.size(), 1u);
    EXPECT_EQ(rec.descriptors[0], RockBandGuitar);
    ASSERT_EQ(rec.queued.size(), 1u);
    EXPECT_EQ(rec.queued[0], (Bytes{0x06, 0x20, 0x01, 0x02, 0x01, 0x00}));
    EXPECT_TRUE(dev.auth_complete_sent);
}

// Several fragments in one transfer are reassembled in order
TEST_F(GipDevice, FragmentsCoalescedInOneTransfer)
{
    const Bytes &md = pb::PDP_DRUMS_METADATA;
    auto frags = fragments(GIP_DEVICE_DESCRIPTOR, 0x07, md);
    frags.push_back(fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x07, md.size()));
    Bytes all;
    for (auto &f : frags)
        all.insert(all.end(), f.begin(), f.end());
    feed(all);
    ASSERT_EQ(rec.descriptors.size(), 1u);
    EXPECT_EQ(rec.descriptors[0], RockBandDrums);
}

// In Xbox One mode the console authenticates the controller through the emulated device, so the
// host must not fake auth-done
TEST_F(GipDevice, XboxOneModeLeavesAuthToTheConsole)
{
    ConfigManager::instance().set_current_mode(ModeXboxOne);
    for (auto &f : fragments(GIP_DEVICE_DESCRIPTOR, 0x02, pb::MICROSOFT_GAMEPAD_METADATA))
        feed(f);
    feed(fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x02, pb::MICROSOFT_GAMEPAD_METADATA.size()));
    ASSERT_EQ(rec.descriptors.size(), 1u);
    EXPECT_EQ(rec.descriptors[0], Gamepad);
    EXPECT_TRUE(rec.queued.empty());
    EXPECT_FALSE(dev.auth_complete_sent);
}

// The controller's security messages go back to the console when the emulated device is relaying
TEST_F(GipDevice, AuthResponsesAreRelayedToTheConsole)
{
    std::vector<Bytes> relayed;
    auth_broker.register_response_handler(ModeXboxOne, [&](XGIPProtocol *p) {
        relayed.emplace_back(p->getData(), p->getData() + p->getDataLength());
    });
    Bytes message = message_bytes(150, 4);
    for (auto &f : fragments(GIP_AUTH, 0x09, message))
        feed(f);
    EXPECT_TRUE(relayed.empty());
    feed(fragment_complete(GIP_AUTH, 0x09, message.size()));
    ASSERT_EQ(relayed.size(), 1u);
    EXPECT_EQ(relayed[0], message);
    EXPECT_FALSE(dev.auth_complete_sent);
}

// [MS-GIPUSB] Table 30: status bits 7:6 are the power level, 00 = powering off / resetting
TEST_F(GipDevice, StatusPoweringOffDisconnects)
{
    feed(packet(GIP_STATUS, FLAG_SYSTEM, 0x01, {0x80, 0x00, 0x00, 0x00}));
    EXPECT_EQ(rec.disconnects, 0);
    feed(packet(GIP_STATUS, FLAG_SYSTEM, 0x02, {0x00, 0x00, 0x00, 0x00}));
    EXPECT_EQ(rec.disconnects, 1);
}

// The guide button comes as message 0x07 ([MS-GIPUSB] 3.1.5.5.6; the 0x20 report "does not
// include the status for the Guide or Share buttons"): xone gip_pkt_virtual_key is {down, key}
// with key GIP_VKEY_LEFT_WIN (0x5B); xpad reports BTN_MODE from the first payload byte.
TEST_F(GipDevice, GuideButtonFromVirtualKey)
{
    dev.subtype = Gamepad;
    feed(packet(GIP_INPUT_REPORT, 0x00, 0x01, Bytes(14, 0)));
    EXPECT_FALSE(digital(SubType_Gamepad, gamepad_button(Gamepad_Guide)));
    feed(packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM, 0x01, {0x01, 0x5B}));
    EXPECT_TRUE(digital(SubType_Gamepad, gamepad_button(Gamepad_Guide)));
    // Input reports in between don't release it
    feed(packet(GIP_INPUT_REPORT, 0x00, 0x02, Bytes(14, 0)));
    EXPECT_TRUE(digital(SubType_Gamepad, gamepad_button(Gamepad_Guide)));
    feed(packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM, 0x02, {0x00, 0x5B}));
    EXPECT_FALSE(digital(SubType_Gamepad, gamepad_button(Gamepad_Guide)));
}

// The Guitar Hero Live guitar's inputs are the PS3-style 0x21 report (PlasticBand 6-Fret
// Guitar/Xbox One.md, byte 0 bit 1 = Black 1). The guide state the host keeps for gamepads (byte 0
// bit 1 of the gamepad layout) must not leak into that layout.
TEST_F(GipDevice, GhlVirtualKeyLeavesTheFretsAlone)
{
    dev.subtype = LiveGuitar;
    Bytes report(27, 0);
    report[2] = 0x0F; // d-pad centred
    report[4] = 0x80; // strum bar at rest
    feed(packet(GIP_HID_REPORT, 0x00, 0x01, report));
    ASSERT_FALSE(digital(SubType_LiveGuitar, ghl_button(GuitarHeroLiveGuitar_Black1)));
    feed(packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM, 0x01, {0x01, 0x5B}));
    EXPECT_FALSE(digital(SubType_LiveGuitar, ghl_button(GuitarHeroLiveGuitar_Black1)));
}

// [MS-GIPUSB] 3.1.5.1: while a reliable transfer is incomplete the host ACKs the contiguous data
// received, no more often than every 100 ms and at most 8 times without a response, and gives up
// after the 1000 ms reliable message timeout. 3.1.2: the host requests metadata again.
TEST_F(GipDevice, StalledTransferHeartbeatAcksThenRetries)
{
    const Bytes &md = pb::PDP_JAGUAR_METADATA;
    auto frags = fragments(GIP_DEVICE_DESCRIPTOR, 0x04, md);
    feed(frags[0]);
    rec.acks.clear();
    rec.queued.clear();

    update(1000); // first tick after the fragment starts the clocks
    update(1099);
    EXPECT_TRUE(rec.queued.empty());
    update(1100);
    ASSERT_EQ(rec.queued.size(), 1u);
    EXPECT_EQ(rec.queued[0], ack(GIP_DEVICE_DESCRIPTOR, 0x04, 58, md.size() - 58));

    for (uint32_t t = 1100; t < 1999; t += 10)
        update(t);
    size_t heartbeats = rec.queued.size();
    EXPECT_LE(heartbeats, 8u);
    EXPECT_GE(heartbeats, 8u); // every 100 ms from 1100 to 1900 would be 9; capped at 8
    for (auto &q : rec.queued)
    {
        ASSERT_FALSE(q.empty());
        EXPECT_EQ(q[0], GIP_ACK_RESPONSE);
    }

    rec.queued.clear();
    update(2000);
    EXPECT_FALSE(dev.incoming_chunk_pending);
    ASSERT_EQ(rec.queued.size(), 1u);
    Header retry = decode(rec.queued[0]);
    EXPECT_EQ(retry.type, GIP_DEVICE_DESCRIPTOR);
    EXPECT_EQ(retry.flags, FLAG_SYSTEM);
    EXPECT_EQ(retry.length, 0u);
}

// A fragment arriving resets the reliable message timeout
TEST_F(GipDevice, FragmentsKeepTheTransferAlive)
{
    const Bytes &md = pb::GHL_DONGLE_METADATA;
    auto frags = fragments(GIP_DEVICE_DESCRIPTOR, 0x04, md);
    uint32_t now = 0;
    for (auto &f : frags)
    {
        feed(f);
        update(now);
        now += 900;
        update(now);
    }
    EXPECT_TRUE(dev.incoming_chunk_pending);
    feed(fragment_complete(GIP_DEVICE_DESCRIPTOR, 0x04, md.size()));
    ASSERT_EQ(rec.descriptors.size(), 1u);
    EXPECT_EQ(rec.descriptors[0], LiveGuitar);
}

// A fragmented message the host sends (a relayed security message) goes out one fragment per
// update, holding after each ACME fragment until the controller ACKs it (xone
// gip_handle_pkt_acknowledge only continues on an ACK) or XGIP_ACK_WAIT_TIMEOUT passes
TEST_F(GipDevice, OutgoingFragmentsWaitForAcks)
{
    Bytes message = message_bytes(218, 5);
    XGIPProtocol *out = dev.outgoing_xgip;
    out->setAttributes(GIP_AUTH, 0x03, 1, 1, 0);
    out->setData(message.data(), message.size());
    uint8_t *first = out->generatePacket();
    Recorder::queue_packet(&rec, first, out->getPacketLength());

    update(0); // fragment 2
    update(1); // fragment 3
    update(2); // fragment 4 (last data, ACME)
    ASSERT_EQ(rec.queued.size(), 4u);
    EXPECT_TRUE(decode(rec.queued[3]).flags & FLAG_ACME);
    update(3);
    update(1000);
    EXPECT_EQ(rec.queued.size(), 4u) << "sent the complete fragment before the ACK";

    feed(ack(GIP_AUTH, 0x03, 218, 0));
    update(1001);
    ASSERT_EQ(rec.queued.size(), 5u);
    EXPECT_EQ(rec.queued[4], fragment_complete(GIP_AUTH, 0x03, 218));
    update(1002);
    EXPECT_EQ(rec.queued.size(), 5u);

    Bytes reassembled;
    for (size_t i = 0; i < 4; i++)
    {
        Bytes p = payload_of(rec.queued[i]);
        reassembled.insert(reassembled.end(), p.begin(), p.end());
    }
    EXPECT_EQ(reassembled, message);
}

TEST_F(GipDevice, AckWaitTimesOut)
{
    Bytes message = message_bytes(100, 5);
    XGIPProtocol *out = dev.outgoing_xgip;
    out->setAttributes(GIP_AUTH, 0x03, 1, 1, 0);
    out->setData(message.data(), message.size());
    uint8_t *first = out->generatePacket();
    Recorder::queue_packet(&rec, first, out->getPacketLength());
    update(10); // last data fragment, waits for its ACK
    ASSERT_EQ(rec.queued.size(), 2u);
    update(10 + XGIP_ACK_WAIT_TIMEOUT - 1);
    EXPECT_EQ(rec.queued.size(), 2u);
    update(10 + XGIP_ACK_WAIT_TIMEOUT);
    EXPECT_EQ(rec.queued.size(), 3u);
}

// Init packets for a standard pad, matching xpad's xboxone_power_on, xboxone_led_on (pattern
// 0x01 = on, brightness 0x14) and a stop-all rumble ([MS-GIPUSB] Table 56: flags 0x00, 9 bytes,
// all four motors; PlasticBand Xbox One Base: duration 0xFF, delay 0x00, repeat 0xEB). Set Device
// State and LED share the global sequence pool, rumble has its own (Table 15).
TEST_F(GipDevice, PowerOnSequenceForAGamepad)
{
    dev.subtype = Gamepad;
    gip_send_power_on_sequence(&dev);
    ASSERT_EQ(rec.queued.size(), 3u);
    EXPECT_EQ(rec.queued[0], (Bytes{0x05, 0x20, 0x01, 0x01, 0x00}));
    EXPECT_EQ(rec.queued[1], (Bytes{0x0A, 0x20, 0x02, 0x03, 0x00, 0x01, 0x14}));
    EXPECT_EQ(rec.queued[2], (Bytes{0x09, 0x00, 0x01, 0x09, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xEB}));
}

TEST_F(GipDevice, PowerOnSequenceForRockBandInstruments)
{
    for (SubType subtype : {RockBandGuitar, RockBandDrums})
    {
        rec.queued.clear();
        gip_sequence_pool_init(&dev.tx_sequence_pools);
        dev.subtype = subtype;
        gip_send_power_on_sequence(&dev);
        ASSERT_EQ(rec.queued.size(), 2u);
        EXPECT_EQ(rec.queued[0], (Bytes{0x05, 0x20, 0x01, 0x01, 0x00}));
        EXPECT_EQ(rec.queued[1], (Bytes{0x0A, 0x20, 0x02, 0x03, 0x00, 0x01, 0x14}));
    }
}

// PlasticBand 6-Fret Guitar/Xbox One.md: the GHL guitar needs message 0x22 sub-command 0x02
// (bytes 0x08, 0x0A, 0x00 x5) "every 8 seconds for inputs to work correctly". 0x22 is a vendor
// message declared in its metadata (flags 0x00).
TEST_F(GipDevice, GhlKeepAlive)
{
    const Bytes poke = {0x02, 0x08, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00};
    dev.subtype = LiveGuitar;
    gip_send_power_on_sequence(&dev);
    ASSERT_EQ(rec.queued.size(), 1u);
    EXPECT_EQ(rec.queued[0], concat({{0x22, 0x00, 0x01, 0x08}, poke}));
    rec.queued.clear();

    update(1000);
    update(8999);
    EXPECT_TRUE(rec.queued.empty());
    update(9000);
    ASSERT_EQ(rec.queued.size(), 1u);
    EXPECT_EQ(rec.queued[0], concat({{0x22, 0x00, 0x02, 0x08}, poke}));
    update(16999);
    EXPECT_EQ(rec.queued.size(), 1u);
    update(17000);
    EXPECT_EQ(rec.queued.size(), 2u);
}

// --- gip_report_queue ---

TEST(GipReportQueue, FifoWithAcksJumpingTheQueue)
{
    gip_report_queue_t *q = gip_report_queue_create();
    const uint8_t a[] = {1}, b[] = {2}, c[] = {3};
    EXPECT_TRUE(gip_report_queue_push(q, a, 1));
    EXPECT_TRUE(gip_report_queue_push(q, b, 1));
    EXPECT_TRUE(gip_report_queue_push_front(q, c, 1));
    std::vector<uint8_t> order;
    while (!gip_report_queue_empty(q))
    {
        order.push_back(gip_report_queue_front(q)->report[0]);
        gip_report_queue_pop(q);
    }
    EXPECT_EQ(order, (std::vector<uint8_t>{3, 1, 2}));
    gip_report_queue_destroy(q);
}

TEST(GipReportQueue, FullQueueRejects)
{
    gip_report_queue_t *q = gip_report_queue_create();
    uint8_t x = 0;
    int pushed = 0;
    while (gip_report_queue_push(q, &x, 1))
        pushed++;
    EXPECT_EQ(pushed, 8);
    EXPECT_FALSE(gip_report_queue_push_front(q, &x, 1));
    EXPECT_FALSE(gip_report_queue_push(q, &x, 0));
    gip_report_queue_destroy(q);
}
