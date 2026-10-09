// src/devices/usb/host/xone_host.cpp: the USB host driver for Xbox One controllers, playing the
// controller through the fake TinyUSB host in support/gip (IN transfers are written into the
// buffer the driver armed, OUT transfers are recorded).
#include <gtest/gtest.h>

#include "devices/usb.hpp"
#include "devices/usb/host/xone_host.h"
#include "gip_fakes.hpp"
#include "gip_test_support.hpp"
#include "pico/time.h"
#include "plasticband_metadata.hpp"
#include "usb/auth_broker.h"
#include "xgip_protocol.h"

using namespace gip_test;
namespace pb = gip_test::plasticband;

namespace
{
constexpr uint8_t ADDR = 1;
constexpr uint8_t EP_IN = 0x81;
constexpr uint8_t EP_OUT = 0x02;

// [MS-GIPUSB] Table 9: GIP data interface 0, class 0xFF, subclass 0x47, protocol 0xD0, with a
// 64 byte interrupt IN and OUT endpoint
const uint8_t GIP_INTERFACE[] = {
    0x09, 0x04, 0x00, 0x00, 0x02, 0xFF, 0x47, 0xD0, 0x00,
    0x07, 0x05, EP_OUT, 0x03, 0x40, 0x00, 0x04,
    0x07, 0x05, EP_IN, 0x03, 0x40, 0x00, 0x04,
};

class XoneHost : public ::testing::Test
{
protected:
    std::shared_ptr<UsbHostDevice> usb;
    std::shared_ptr<XboxOneHost> host;

    void SetUp() override
    {
        gip_fake::reset();
        usb = std::make_shared<UsbHostDevice>(ADDR, 0);
        host_devices[ADDR] = usb;
        uint16_t len = 0;
        auto itf = XboxOneHost::open(usb, (tusb_desc_interface_t const *)GIP_INTERFACE, sizeof(GIP_INTERFACE), &len);
        ASSERT_TRUE(itf);
        EXPECT_EQ(len, sizeof(GIP_INTERFACE));
        usb->host_devices_by_itf[0] = itf;
        host = std::static_pointer_cast<XboxOneHost>(itf);
        host->set_config();
    }
    void TearDown() override
    {
        if (host)
        {
            host->disconnect();
        }
        host.reset();
        usb.reset();
        gip_fake::reset();
    }

    // The controller sends one transfer
    void from_controller(const Bytes &wire)
    {
        ASSERT_NE(gip_fake::host_in_buffer, nullptr);
        ASSERT_LE(wire.size(), gip_fake::host_in_size);
        memcpy(gip_fake::host_in_buffer, wire.data(), wire.size());
        host->xfer_cb(EP_IN, XFER_RESULT_SUCCESS, wire.size());
    }
    // The metadata transfer, with the driver's main loop running between USB transfers;
    // returns what the driver sent meanwhile
    std::vector<Bytes> send_metadata(const Bytes &md, uint8_t seq = 0x01)
    {
        std::vector<Bytes> sent;
        auto frags = fragments(GIP_DEVICE_DESCRIPTOR, seq, md);
        frags.push_back(fragment_complete(GIP_DEVICE_DESCRIPTOR, seq, md.size()));
        for (auto &f : frags)
        {
            from_controller(f);
            for (auto &p : drain())
                sent.push_back(p);
        }
        return sent;
    }
    // Run the driver until it has nothing left to send; returns what it sent
    std::vector<Bytes> drain()
    {
        std::vector<Bytes> sent;
        for (int i = 0; i < 64; i++)
        {
            size_t before = gip_fake::host_out.size();
            gip_fake_time::advance_ms(1);
            host->update(false, false);
            if (gip_fake::host_out.size() == before && gip_report_queue_empty(host->m_report_queue))
                break;
        }
        for (auto &t : gip_fake::host_out)
        {
            EXPECT_EQ(t.ep, EP_OUT);
            sent.push_back(t.data);
        }
        gip_fake::host_out.clear();
        return sent;
    }
    static std::vector<Bytes> without_acks(const std::vector<Bytes> &packets)
    {
        std::vector<Bytes> out;
        for (auto &p : packets)
            if (!p.empty() && p[0] != GIP_ACK_RESPONSE)
                out.push_back(p);
        return out;
    }
};

proto_Output rb_button(proto_RockBandGuitarButtonType b)
{
    proto_Output o = {};
    o.which_mapping = proto_Output_rbButton_tag;
    o.mapping.rbButton = b;
    return o;
}
} // namespace

TEST_F(XoneHost, OpensTheGipDataInterface)
{
    EXPECT_EQ(gip_fake::enumerating.size(), 1u);
    EXPECT_EQ(gip_fake::host_in_arms, 1);
    EXPECT_EQ(gip_fake::host_in_size, 64);
    EXPECT_TRUE(host->has_rumble());
    EXPECT_TRUE(host->has_player_led());
}

TEST_F(XoneHost, IgnoresOtherVendorInterfaces)
{
    uint8_t xinput[sizeof(GIP_INTERFACE)];
    memcpy(xinput, GIP_INTERFACE, sizeof(xinput));
    xinput[6] = 0x5D; // XInput subclass
    xinput[7] = 0x01;
    uint16_t len = 0;
    EXPECT_FALSE(XboxOneHost::open(usb, (tusb_desc_interface_t const *)xinput, sizeof(xinput), &len));
}

// The metadata exchange from the host's side: every ACME fragment is acked on the OUT endpoint
// (xone gip_acknowledge_pkt layout), the device is identified from its metadata, and the
// interface becomes assignable with the console auth relay registered
TEST_F(XoneHost, MetadataExchange)
{
    const Bytes &md = pb::MADCATZ_STRATOCASTER_METADATA;
    auto sent = send_metadata(md, 0x05);
    ASSERT_GE(sent.size(), 2u);
    EXPECT_EQ(sent[0], ack(GIP_DEVICE_DESCRIPTOR, 0x05, 58, md.size() - 58));
    EXPECT_EQ(sent[1], ack(GIP_DEVICE_DESCRIPTOR, 0x05, md.size(), 0));
    EXPECT_EQ(host->subtype(), RockBandGuitar);
    EXPECT_TRUE(gip_fake::enumerating.empty());
    ASSERT_EQ(gip_fake::assignable.size(), 1u);
    EXPECT_EQ(gip_fake::assignable[0].get(), host.get());
    EXPECT_TRUE(auth_broker.has_handler(ModeXboxOne));
    EXPECT_EQ(gip_fake::delayed_init_calls, 1);
    // The IN endpoint is re-armed after every transfer
    EXPECT_GT(gip_fake::host_in_arms, 5);
}

// The packets the host sends to bring a controller up, in order: the Metadata Request answering
// its Hello ([MS-GIPUSB] Figure 3; xone gip_request_identification), then for a Rock Band
// instrument Set Device State START and the guide LED on (xpad xboxone_power_on /
// xboxone_led_on), then xpad's xboxone_auth_done. Metadata / Set State / LED share the global
// sequence pool, security has its own ([MS-GIPUSB] Table 15). Each is queued whole (see
// GipDevice.QueuedPacketsAreWhole).
TEST_F(XoneHost, InitPacketsReachTheController)
{
    from_controller(packet(GIP_ANNOUNCE, FLAG_SYSTEM, 0x01, pb::MADCATZ_STRATOCASTER_HELLO));
    auto sent = drain();
    ASSERT_EQ(sent.size(), 1u);
    EXPECT_EQ(sent[0], (Bytes{0x04, 0x20, 0x01, 0x00}));

    auto init = without_acks(send_metadata(pb::MADCATZ_STRATOCASTER_METADATA));
    ASSERT_EQ(init.size(), 3u);
    EXPECT_EQ(init[0], (Bytes{0x05, 0x20, 0x02, 0x01, 0x00}));
    EXPECT_EQ(init[1], (Bytes{0x0A, 0x20, 0x03, 0x03, 0x00, 0x01, 0x14}));
    EXPECT_EQ(init[2], (Bytes{0x06, 0x20, 0x01, 0x02, 0x01, 0x00}));
}

TEST_F(XoneHost, DecodesInputReports)
{
    send_metadata(pb::PDP_JAGUAR_METADATA);
    Bytes payload(10, 0);
    payload[5] = 0x01 | 0x10; // green, orange (PlasticBand: byte 5 upper fret bitmask)
    from_controller(packet(GIP_INPUT_REPORT, 0x00, 0x01, payload));
    proto_Output green = rb_button(RockBandGuitar_Green), orange = rb_button(RockBandGuitar_Orange), red = rb_button(RockBandGuitar_Red);
    EXPECT_TRUE(host->tick_digital(green));
    EXPECT_TRUE(host->tick_digital(orange));
    EXPECT_FALSE(host->tick_digital(red));
}

// A failed OUT transfer keeps the packet queued for the next update
TEST_F(XoneHost, RetriesBusyOutEndpoint)
{
    from_controller(packet(GIP_VIRTUAL_KEYCODE, FLAG_SYSTEM | FLAG_ACME, 0x09, {0x01, 0x5B}));
    gip_fake::host_out_fail = true;
    host->update(false, false);
    EXPECT_TRUE(gip_fake::host_out.empty());
    gip_fake::host_out_fail = false;
    auto sent = drain();
    ASSERT_EQ(sent.size(), 1u);
    EXPECT_EQ(sent[0], ack(GIP_VIRTUAL_KEYCODE, 0x09, 2, 0));
}

// Rumble: [MS-GIPUSB] Table 56 Direct Motor Command - type 0x09, flags 0x00, 9 bytes: 0x00, motor
// bitmap ([3] left impulse, [2] right impulse, [1] left vibration, [0] right vibration), left /
// right impulse levels, left / right vibration levels in percent, duration (0 cancels), delay,
// repeat. PlasticBand's Xbox One template and xpad (GIP_MOTOR_R BIT(0), GIP_MOTOR_L BIT(1),
// GIP_MOTOR_RT BIT(2), GIP_MOTOR_LT BIT(3)) agree on the bitmap. The levels go in the vibration
// motor bytes, so src/devices/usb/host/xone_host.cpp XboxOneHost::set_rumble has to enable the
// vibration motors (0x01 / 0x02) in the bitmap, not just the trigger impulse motors (0x0C).
TEST_F(XoneHost, RumbleDrivesTheVibrationMotors)
{
    host->set_rumble(255, 128);
    auto sent = drain();
    ASSERT_EQ(sent.size(), 1u);
    Header h = decode(sent[0]);
    EXPECT_EQ(h.type, GIP_CMD_RUMBLE);
    EXPECT_EQ(h.flags, 0x00);
    EXPECT_NE(h.sequence, 0);
    Bytes p = payload_of(sent[0]);
    ASSERT_EQ(p.size(), 9u);
    EXPECT_EQ(p[1] & 0x03, 0x03) << "vibration motors not enabled, bitmap " << (int)p[1];
    EXPECT_EQ(p[4], 100);
    EXPECT_EQ(p[5], 50);
    EXPECT_NE(p[6], 0);

    host->set_rumble(0, 0);
    sent = drain();
    ASSERT_EQ(sent.size(), 1u);
    p = payload_of(sent[0]);
    ASSERT_EQ(p.size(), 9u);
    EXPECT_EQ(p[4], 0);
    EXPECT_EQ(p[5], 0);
}

// Guide button LED: [MS-GIPUSB] Table 41 - type 0x0A, flags 0x20 (a system message; 2.2.10.2: a
// message without the System flag "MUST be defined in the Metadata", and 0x0A isn't in a
// controller's message list), payload 0x00, pattern, intensity. xpad's xboxone_led_on is
// GIP_CMD_LED, GIP_OPT_INTERNAL, seq, 3, 0x00, GIP_LED_ON, 0x14. (src/devices/usb/host/xone_host.cpp
// XboxOneHost::send_feedback_packet builds from a reset XGIPProtocol, so set_player_led asks for
// the System flag explicitly.)
TEST_F(XoneHost, PlayerLedIsASystemMessage)
{
    host->set_player_led(1);
    auto sent = drain();
    ASSERT_EQ(sent.size(), 1u);
    Header h = decode(sent[0]);
    EXPECT_EQ(h.type, GIP_CMD_LED_ON);
    EXPECT_EQ(h.flags, FLAG_SYSTEM);
    EXPECT_EQ(payload_of(sent[0]), (Bytes{0x00, 0x01, 0x14}));
}

// Security messages from the console are passed to the controller unchanged, re-fragmented with
// the same framing a real host uses (see xgip_fragment_test.cpp), and the controller's reply goes
// back to the console. send_report_from_host queues its first fragment whole (see
// GipDevice.QueuedPacketsAreWhole).
TEST_F(XoneHost, ConsoleSecurityMessagesReachTheController)
{
    send_metadata(pb::MICROSOFT_GAMEPAD_METADATA);
    Bytes message = message_bytes(218, 11);
    XGIPProtocol from_console;
    for (auto &f : fragments(GIP_AUTH, 0x07, message))
        ASSERT_TRUE(from_console.parse(f.data(), f.size()));
    Bytes done = fragment_complete(GIP_AUTH, 0x07, message.size());
    ASSERT_TRUE(from_console.parse(done.data(), done.size()));
    ASSERT_TRUE(auth_broker.forward_auth(ModeXboxOne, &from_console));

    Bytes reassembled;
    for (int round = 0; round < 10; round++)
    {
        for (auto &p : drain())
        {
            Header h = decode(p);
            if (h.type != GIP_AUTH)
                continue;
            if (h.length)
            {
                Bytes payload = payload_of(p);
                reassembled.insert(reassembled.end(), payload.begin(), payload.end());
            }
            if (h.flags & FLAG_ACME)
                from_controller(ack(GIP_AUTH, h.sequence, reassembled.size(), message.size() - reassembled.size()));
        }
    }
    EXPECT_EQ(reassembled, message);
}

// The controller's reply to a relayed security message goes back to the console
TEST_F(XoneHost, ControllerSecurityRepliesGoToTheConsole)
{
    std::vector<Bytes> to_console;
    auth_broker.register_response_handler(ModeXboxOne, [&](XGIPProtocol *p) {
        to_console.emplace_back(p->getData(), p->getData() + p->getDataLength());
    });
    send_metadata(pb::MICROSOFT_GAMEPAD_METADATA);
    Bytes reply = message_bytes(40, 2);
    from_controller(packet(GIP_AUTH, FLAG_SYSTEM, 0x03, reply));
    ASSERT_EQ(to_console.size(), 1u);
    EXPECT_EQ(to_console[0], reply);
}
