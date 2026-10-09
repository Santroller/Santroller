#include <gtest/gtest.h>
#include <vector>
#include "protocols/ble_midi.hpp"

namespace
{
using Bytes = std::vector<uint8_t>;

Bytes unwrap(const Bytes &packet)
{
    Bytes out;
    ble_midi_unwrap(packet.data(), packet.size(), [&](uint8_t byte)
                    { out.push_back(byte); });
    return out;
}
} // namespace

TEST(BleMidi, SingleMessage)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0x90, 0x3C, 0x7F}), (Bytes{0x90, 0x3C, 0x7F}));
}

TEST(BleMidi, SeveralMessagesWithTheirOwnTimestamps)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0x90, 0x3C, 0x7F, 0x81, 0x80, 0x3C, 0x00}),
              (Bytes{0x90, 0x3C, 0x7F, 0x80, 0x3C, 0x00}));
}

TEST(BleMidi, RunningStatusWithoutTimestamps)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0x90, 0x3C, 0x7F, 0x3E, 0x7F, 0x40, 0x7F}),
              (Bytes{0x90, 0x3C, 0x7F, 0x3E, 0x7F, 0x40, 0x7F}));
}

TEST(BleMidi, RunningStatusAfterATimestamp)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0x90, 0x3C, 0x7F, 0x82, 0x3E, 0x7F}),
              (Bytes{0x90, 0x3C, 0x7F, 0x3E, 0x7F}));
}

TEST(BleMidi, SysexAcrossPackets)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0xF0, 0x01, 0x02}), (Bytes{0xF0, 0x01, 0x02}));
    // the continuation has data straight after the header, and the end has its own timestamp
    EXPECT_EQ(unwrap({0x80, 0x03, 0x04, 0x85, 0xF7}), (Bytes{0x03, 0x04, 0xF7}));
}

TEST(BleMidi, RealtimeBetweenMessages)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0xF8, 0x81, 0xB0, 0x07, 0x64}), (Bytes{0xF8, 0xB0, 0x07, 0x64}));
}

TEST(BleMidi, PacketsWithoutAHeaderAreDropped)
{
    EXPECT_TRUE(unwrap({0x40, 0x80, 0x90, 0x3C, 0x7F}).empty());
    EXPECT_TRUE(unwrap({0xC0, 0x80, 0x90, 0x3C, 0x7F}).empty());
    EXPECT_TRUE(unwrap({0x80}).empty());
    EXPECT_TRUE(unwrap({}).empty());
}

TEST(BleMidi, TimestampAtTheEndOfAPacket)
{
    EXPECT_EQ(unwrap({0x80, 0x80, 0x90, 0x3C, 0x7F, 0x81}), (Bytes{0x90, 0x3C, 0x7F}));
}

TEST(BleMidi, UuidsMatchTheSpec)
{
    // 03b80e5a-ede8-4b33-a751-6ce34ec4c700, reversed on the air
    EXPECT_EQ(ble_midi_service_uuid_le[15], 0x03);
    EXPECT_EQ(ble_midi_service_uuid_le[0], 0x00);
    // 7772e5db-3868-4112-a1a9-f2669d106bf3
    EXPECT_EQ(ble_midi_char_uuid[0], 0x77);
    EXPECT_EQ(ble_midi_char_uuid[15], 0xF3);
}
