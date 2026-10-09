// USB MIDI event packets: a cable number / CIN byte then three MIDI bytes, padded with zeros.
#include <gtest/gtest.h>
#include "midi/midi_test_support.hpp"

using namespace midi_test;

namespace
{
Bytes concat(std::initializer_list<Bytes> parts)
{
    Bytes out;
    for (const Bytes &part : parts)
    {
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}
} // namespace

TEST(MidiUsbPackets, EncoderMatchesTheSpec)
{
    // examples from the USB MIDI 1.0 spec, table 4-2 (cable 1)
    EXPECT_EQ(usb_message(1, {0x90, 0x3C, 0x64}), (Bytes{0x19, 0x90, 0x3C, 0x64}));
    EXPECT_EQ(usb_message(1, {0xC0, 0x05}), (Bytes{0x1C, 0xC0, 0x05, 0x00}));
    EXPECT_EQ(usb_message(1, {0xF8}), (Bytes{0x1F, 0xF8, 0x00, 0x00}));
    EXPECT_EQ(usb_message(1, {0xF0, 0x01, 0x02, 0x03, 0x04, 0xF7}), (Bytes{0x14, 0xF0, 0x01, 0x02, 0x17, 0x03, 0x04, 0xF7}));
    EXPECT_EQ(usb_message(1, {0xF0, 0x01, 0xF7}), (Bytes{0x17, 0xF0, 0x01, 0xF7}));
    EXPECT_EQ(usb_message(1, {0xF0, 0x01, 0x02, 0xF7}), (Bytes{0x14, 0xF0, 0x01, 0x02, 0x15, 0xF7, 0x00, 0x00}));
}

TEST(MidiUsbPackets, EmptyPacketsAreSkipped)
{
    MidiHarness h(true);
    h.feed(concat({{0x00, 0x00, 0x00, 0x00}, usb_message(0, {0x90, 0x3C, 0x64}), {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, usb_message(0, {0xB0, 0x07, 0x10})}));
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xB0, 0x07, 0x10}}));
    EXPECT_EQ(h.stream.available(), 0u);
}

TEST(MidiUsbPackets, OnlyEmptyPackets)
{
    MidiHarness h(true);
    h.feed(Bytes(64, 0x00));
    EXPECT_TRUE(h.messages.empty());
    EXPECT_TRUE(h.marked.empty());
    EXPECT_EQ(h.stream.available(), 0u);
    // and the next real packet still lines up
    h.feed(usb_message(0, {0x90, 0x3C, 0x64}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
}

TEST(MidiUsbPackets, PaddingAfterShortMessagesIsDiscarded)
{
    MidiHarness h(true);
    // padding bytes are zero, which would otherwise look like data bytes
    h.feed(concat({usb_message(0, {0xC0, 0x05}), usb_message(0, {0xF8}), usb_message(0, {0xF6}), usb_message(0, {0xF1, 0x12}), usb_message(0, {0x90, 0x3C, 0x64})}));
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xC0, 0x05}, {0xF8}, {0xF6}, {0xF1, 0x12}, {0x90, 0x3C, 0x64}}));
    EXPECT_EQ(h.stream.available(), 0u);
}

TEST(MidiUsbPackets, PaddingIsDiscardedEvenWithRunningStatusSet)
{
    MidiHarness h(true);
    // after a note on, a zero padding byte read as running status would be a note on for note 0
    h.feed(concat({usb_message(0, {0x90, 0x3C, 0x64}), usb_message(0, {0xF8}), usb_message(0, {0xC0, 0x01})}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x00), 0);
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xF8}, {0xC0, 0x01}}));
}

TEST(MidiUsbPackets, SysexEndingInEachKindOfPacket)
{
    MidiHarness h(true);
    // ends with 1, 2 and 3 bytes in the last packet (CIN 5, 6 and 7)
    Bytes ends_1 = {0xF0, 0x01, 0x02, 0xF7};
    Bytes ends_2 = {0xF0, 0x01, 0x02, 0x03, 0xF7};
    Bytes ends_3 = {0xF0, 0x01, 0x02, 0x03, 0x04, 0xF7};
    h.feed(concat({usb_message(0, ends_1), usb_message(0, {0x90, 0x3C, 0x64}), usb_message(0, ends_2), usb_message(0, ends_3), usb_message(0, {0x80, 0x3C, 0x00})}));
    EXPECT_EQ(h.messages, (std::vector<Bytes>{ends_1, {0x90, 0x3C, 0x64}, ends_2, ends_3, {0x80, 0x3C, 0x00}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0);
}

TEST(MidiUsbPackets, SysexAcrossTransfers)
{
    MidiHarness h(true);
    Bytes packets = usb_message(0, {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_SQUIER, 0x01, 0x06, 0x2B, 0xF7});
    // one packet per transfer
    for (size_t i = 0; i < packets.size(); i += 4)
    {
        h.feed(Bytes(packets.begin() + i, packets.begin() + i + 4));
    }
    // low E string (string 6, open note 0x28) at fret 3
    EXPECT_EQ(h.midi.pro_guitar_frets()[5], 3);
    EXPECT_TRUE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_SQUIER));
}

TEST(MidiUsbPackets, PacketsSplitAcrossTransfers)
{
    MidiHarness h(true);
    Bytes packets = concat({usb_message(0, {0x90, 0x3C, 0x64}), {0x00, 0x00, 0x00, 0x00}, usb_message(0, {0xC0, 0x01}), usb_message(0, {0xF0, 0x01, 0x02, 0x03, 0xF7}), usb_message(0, {0xB0, 0x07, 0x10})});
    // split at every packet boundary, as the host stack delivers whole packets
    for (size_t split = 4; split < packets.size(); split += 4)
    {
        MidiHarness split_harness(true);
        split_harness.feed(Bytes(packets.begin(), packets.begin() + split));
        split_harness.feed(Bytes(packets.begin() + split, packets.end()));
        EXPECT_EQ(split_harness.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xC0, 0x01}, {0xF0, 0x01, 0x02, 0x03, 0xF7}, {0xB0, 0x07, 0x10}}))
            << "split at " << split;
    }
}

TEST(MidiUsbPackets, CablesKeepSeparateMessages)
{
    MidiHarness h(true, 2);
    Bytes sysex = usb_message(0, {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x03, 0x3A, 0xF7});
    // a note on cable 1 goes between the packets of a sysex on cable 0
    h.feed(concat({Bytes(sysex.begin(), sysex.begin() + 4), usb_message(1, {0x90, 0x3C, 0x64}), Bytes(sysex.begin() + 4, sysex.end())}));
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x03, 0x3A, 0xF7}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    // G string (string 3, open note 0x37) at fret 3
    EXPECT_EQ(h.midi.pro_guitar_frets()[2], 3);
}

TEST(MidiUsbPackets, CablesShareTheChannelState)
{
    MidiHarness h(true, 4);
    // cables are separate streams, but the notes they send end up in the same place
    h.feed(concat({usb_message(0, {0x90, 0x3C, 0x64}), usb_message(3, {0x90, 0x3E, 0x50}), usb_message(2, {0x80, 0x3C, 0x00})}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x50);
}

TEST(MidiUsbPackets, CablesPastTheLastOneUseCableZero)
{
    MidiHarness h(true, 1);
    h.feed(concat({usb_message(5, {0x90, 0x3C, 0x64}), usb_message(15, {0xB0, 0x07, 0x10})}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.midi.read_midi_control_change(0, 0x07), 0x10 << 9);
}

TEST(MidiUsbPackets, ManyPacketsInOneTransfer)
{
    MidiHarness h(true);
    Bytes packets;
    for (uint8_t note = 0; note < 128; note++)
    {
        Bytes packet = usb_message(0, {0x90, note, static_cast<uint8_t>(note | 1)});
        packets.insert(packets.end(), packet.begin(), packet.end());
    }
    h.feed(packets);
    for (uint8_t note = 0; note < 128; note++)
    {
        EXPECT_EQ(h.midi.read_midi_note(0, note), note | 1) << int(note);
    }
}

// USB MIDI 1.0 4: a CIN 0xF packet holds one MIDI byte, the other two are padding. Realtime messages
// are sent as their own packet, so they can arrive between the packets of a sysex message.
TEST(MidiUsbPackets, RealtimePacketInsideSysexAddsNoData)
{
    MidiHarness h(true);
    Bytes sysex = usb_message(0, {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7});
    h.feed(concat({Bytes(sysex.begin(), sysex.begin() + 4), usb_message(0, {0xF8}), Bytes(sysex.begin() + 4, sysex.end())}));
    // high E string (string 1, open note 0x40) at fret 5
    EXPECT_EQ(h.midi.pro_guitar_frets()[0], 5);
    ASSERT_FALSE(h.messages.empty());
    EXPECT_EQ(h.messages.back(), (Bytes{0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7}));
}

// CIN 0 (miscellaneous) and 1 (cable events) are reserved for future use (USB MIDI 1.0, table 4-1)
TEST(MidiUsbPackets, ReservedCinPacketsAreIgnored)
{
    MidiHarness h(true, 2);
    h.feed({0x00, 0x90, 0x3C, 0x64, 0x01, 0x90, 0x3D, 0x64, 0x10, 0x90, 0x3E, 0x64, 0x11, 0xB0, 0x07, 0x10});
    EXPECT_TRUE(h.messages.empty());
    EXPECT_TRUE(h.marked.empty());
    h.feed(usb_message(1, {0x90, 0x40, 0x64}));
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x40, 0x64}}));
}

TEST(MidiUsbPackets, PartialPacketWaitsForTheRest)
{
    MidiHarness h(true, 2);
    Bytes sysex = usb_message(0, {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7});
    // half of a cable 1 packet arrives, then the rest of it, between the packets of a cable 0 sysex
    h.feed(concat({Bytes(sysex.begin(), sysex.begin() + 4), {0x19, 0x91}}));
    EXPECT_TRUE(h.messages.empty());
    EXPECT_EQ(h.stream.available(), 2u);
    h.feed({0x3C, 0x64});
    h.feed(Bytes(sysex.begin() + 4, sysex.end()));
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3C), 0x64);
    EXPECT_EQ(h.midi.pro_guitar_frets()[0], 5);
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x91, 0x3C, 0x64}, {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7}}));
}
