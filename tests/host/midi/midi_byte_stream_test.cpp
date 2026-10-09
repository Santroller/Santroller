// A plain MIDI byte stream, as serial and BLE MIDI deliver it: running status, realtime bytes anywhere,
// and messages split across reads however the bytes happened to arrive.
#include <gtest/gtest.h>
#include "midi/midi_test_support.hpp"

using namespace midi_test;

namespace
{
class MidiByteStream : public ::testing::Test
{
protected:
    MidiHarness h{false};

    bool note_event(uint8_t channel, uint8_t note, uint16_t &sequence)
    {
        uint16_t velocity;
        return h.midi.consume_midi_note_event(channel, note, sequence, velocity);
    }
};
} // namespace

TEST_F(MidiByteStream, RunningStatusNoteOns)
{
    h.feed({0x90, 0x3C, 0x64, 0x3E, 0x50, 0x40, 0x7F});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0x90, 0x3E, 0x50}, {0x90, 0x40, 0x7F}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x50);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x40), 0x7F);
}

TEST_F(MidiByteStream, RunningStatusNoteOnWithZeroVelocityReleases)
{
    // the usual way to send note offs with running status
    h.feed({0x92, 0x3C, 0x64, 0x3E, 0x64, 0x3C, 0x00});
    EXPECT_EQ(h.midi.read_midi_note(2, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_note(2, 0x3E), 0x64);
}

TEST_F(MidiByteStream, RunningStatusControlChanges)
{
    h.feed({0xB5, 0x07, 0x10, 0x0A, 0x20, 0x40, 0x7F});
    EXPECT_EQ(h.midi.read_midi_control_change(5, 0x07), 0x10 << 9);
    EXPECT_EQ(h.midi.read_midi_control_change(5, 0x0A), 0x20 << 9);
    EXPECT_EQ(h.midi.read_midi_control_change(5, 0x40), 0x7F << 9);
}

TEST_F(MidiByteStream, RunningStatusTwoByteMessages)
{
    h.feed({0xC3, 0x01, 0x02, 0x03, 0xD4, 0x10, 0x20});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xC3, 0x01}, {0xC3, 0x02}, {0xC3, 0x03}, {0xD4, 0x10}, {0xD4, 0x20}}));
}

TEST_F(MidiByteStream, NewStatusReplacesRunningStatus)
{
    h.feed({0x90, 0x3C, 0x64, 0xB0, 0x07, 0x10, 0x08, 0x20});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xB0, 0x07, 0x10}, {0xB0, 0x08, 0x20}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x08), 0);
}

TEST_F(MidiByteStream, RealtimeKeepsRunningStatus)
{
    // MIDI 1.0: realtime messages don't affect running status
    h.feed({0x90, 0x3C, 0x64, 0xFE, 0x3E, 0x64, 0xF8, 0x40, 0x64});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xFE}, {0x90, 0x3E, 0x64}, {0xF8}, {0x90, 0x40, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x64);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x40), 0x64);
}

TEST_F(MidiByteStream, RealtimeInsideAMessage)
{
    // realtime bytes can go between any two bytes, and the message carries on around them
    h.feed({0x90, 0xF8, 0x3C, 0xFE, 0x64});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    h.feed({0xB0, 0x07, 0xF8, 0x30});
    EXPECT_EQ(h.midi.read_midi_control_change(0, 0x07), 0x30 << 9);
}

TEST_F(MidiByteStream, RealtimeInsideSysex)
{
    h.feed({0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0xF8, 0x01, 0x01, 0xFE, 0x45, 0xF7});
    // high E string (string 1, open note 0x40) at fret 5
    EXPECT_EQ(h.midi.pro_guitar_frets()[0], 5);
    ASSERT_FALSE(h.messages.empty());
    EXPECT_EQ(h.messages.back(), (Bytes{0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7}));
}

TEST_F(MidiByteStream, ZeroBytesAreData)
{
    // only USB MIDI has empty packets, here a zero is just a data byte
    h.feed({0xB0, 0x00, 0x00, 0x00, 0x05});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xB0, 0x00, 0x00}, {0xB0, 0x00, 0x05}}));
    EXPECT_EQ(h.midi.read_midi_control_change(0, 0x00), 0x05 << 9);
}

TEST_F(MidiByteStream, MessagesSplitAcrossReads)
{
    // one byte per read, so every message is split
    Bytes bytes = {0x90, 0x3C, 0x64, 0x3E, 0x50, 0xB1, 0x07, 0x7F, 0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_SQUIER, 0x01, 0x02, 0x3F, 0xF7, 0xC2, 0x01};
    for (uint8_t byte : bytes)
    {
        h.feed({byte});
    }
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64},
                                              {0x90, 0x3E, 0x50},
                                              {0xB1, 0x07, 0x7F},
                                              {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_SQUIER, 0x01, 0x02, 0x3F, 0xF7},
                                              {0xC2, 0x01}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x50);
    EXPECT_EQ(h.midi.read_midi_control_change(1, 0x07), 0x7F << 9);
    // B string (string 2, open note 0x3B) at fret 4
    EXPECT_EQ(h.midi.pro_guitar_frets()[1], 4);
}

TEST_F(MidiByteStream, MessagesSplitAtEveryPoint)
{
    Bytes bytes = {0x90, 0x3C, 0x64, 0x3E, 0x50, 0xF8, 0xB1, 0x07, 0x7F, 0xF0, 0x01, 0x02, 0xF7, 0x80, 0x3C, 0x00};
    std::vector<Bytes> expected = {{0x90, 0x3C, 0x64}, {0x90, 0x3E, 0x50}, {0xF8}, {0xB1, 0x07, 0x7F}, {0xF0, 0x01, 0x02, 0xF7}, {0x80, 0x3C, 0x00}};
    for (size_t split = 1; split < bytes.size(); split++)
    {
        MidiHarness split_harness(false);
        split_harness.feed(Bytes(bytes.begin(), bytes.begin() + split));
        split_harness.feed(Bytes(bytes.begin() + split, bytes.end()));
        EXPECT_EQ(split_harness.messages, expected) << "split at " << split;
        EXPECT_EQ(split_harness.midi.read_midi_note(0, 0x3C), 0) << "split at " << split;
        EXPECT_EQ(split_harness.midi.read_midi_note(0, 0x3E), 0x50) << "split at " << split;
    }
}

TEST_F(MidiByteStream, ReadingAnEmptyFifoDoesNothing)
{
    h.feed({0x90, 0x3C});
    h.parse();
    h.parse();
    EXPECT_TRUE(h.messages.empty());
    h.feed({0x64});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}}));
}

// MIDI 1.0: sysex and system common messages cancel running status, and data bytes that arrive without a
// valid running status are ignored.
TEST_F(MidiByteStream, SysexCancelsRunningStatus)
{
    h.feed({0x90, 0x3C, 0x64, 0xF0, 0x01, 0xF7, 0x3E, 0x64});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0);
    uint16_t sequence = 0;
    EXPECT_FALSE(note_event(0, 0x3E, sequence));
}

// Same as above, for a system common message
TEST_F(MidiByteStream, SystemCommonCancelsRunningStatus)
{
    h.feed({0x90, 0x3C, 0x64, 0xF6, 0x3E, 0x64});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0);
}

// Joining a stream part way through (e.g. plugging in a serial MIDI device that is already sending) can
// start with data bytes. They are ignored until a status byte arrives.
TEST_F(MidiByteStream, DataBytesBeforeAnyStatusAreIgnored)
{
    h.feed({0x3C, 0x64, 0x91, 0x3E, 0x7F});
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3E), 0x7F);
    EXPECT_TRUE(h.midi.has_midi_channel(1));
    EXPECT_FALSE(h.midi.has_midi_channel(0));
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x91, 0x3E, 0x7F}}));
}

// MIDI 1.0: a status byte (other than realtime) always starts a new message, abandoning an incomplete
// one.
TEST_F(MidiByteStream, StatusByteAbandonsAnIncompleteMessage)
{
    h.feed({0x91, 0x3C, 0x92, 0x3E, 0x7F});
    EXPECT_EQ(h.midi.read_midi_note(2, 0x3E), 0x7F);
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3C), 0);
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x92, 0x3E, 0x7F}}));
}

// MIDI 1.0: any status byte other than realtime ends a sysex message, F7 is optional.
TEST_F(MidiByteStream, StatusByteEndsSysex)
{
    h.feed({0xF0, 0x7D, 0x01, 0x90, 0x3C, 0x64});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xF0, 0x7D, 0x01}, {0x90, 0x3C, 0x64}}));
}

// F7 only means something at the end of a sysex message, on its own it is ignored. Like any system common
// status it still cancels running status.
TEST_F(MidiByteStream, StrayEndOfExclusiveIsIgnored)
{
    h.feed({0x90, 0x3C, 0x64, 0xF7, 0x3E, 0x64, 0x90, 0x40, 0x64});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0x90, 0x40, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0);
}

// F4 and F5 are undefined system common messages and are ignored (MIDI 1.0).
TEST_F(MidiByteStream, UndefinedSystemCommonIsIgnored)
{
    h.feed({0x90, 0x3C, 0x64, 0xF4, 0x90, 0x3E, 0x64, 0xF5, 0x90, 0x40, 0x64});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x64);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x40), 0x64);
}
