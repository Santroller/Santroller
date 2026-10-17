// What input detection reads from the parser: note ons newer than where it started, and the latest
// control change and pitch bend along with the value each had before.
#include <gtest/gtest.h>
#include "midi/midi_test_support.hpp"

using namespace midi_test;

TEST(MidiDetection, WalksNoteOnsAfterTheStartingSequence)
{
    MidiHarness h{false};
    h.feed({0x90, 0x24, 0x40});
    uint16_t sequence = h.midi.midi_note_event_sequence();
    uint8_t channel = 0, note = 0;
    EXPECT_FALSE(h.midi.next_midi_note_event(sequence, channel, note));

    // a drum hit: on and off again before anything looks, then a hit on another channel
    h.feed({0x99, 0x26, 0x50});
    h.feed({0x89, 0x26, 0x00});
    h.feed({0x92, 0x30, 0x10});
    ASSERT_TRUE(h.midi.next_midi_note_event(sequence, channel, note));
    EXPECT_EQ(channel, 9);
    EXPECT_EQ(note, 0x26);
    ASSERT_TRUE(h.midi.next_midi_note_event(sequence, channel, note));
    EXPECT_EQ(channel, 2);
    EXPECT_EQ(note, 0x30);
    EXPECT_FALSE(h.midi.next_midi_note_event(sequence, channel, note));
}

TEST(MidiDetection, NoteOnWithZeroVelocityIsNotAHit)
{
    MidiHarness h{false};
    uint16_t sequence = h.midi.midi_note_event_sequence();
    h.feed({0x90, 0x24, 0x00});
    uint8_t channel = 0, note = 0;
    EXPECT_FALSE(h.midi.next_midi_note_event(sequence, channel, note));
}

TEST(MidiDetection, TracksTheLastControlChange)
{
    MidiHarness h{false};
    uint16_t start = h.midi.last_control_change().sequence;
    h.feed({0xB9, 0x04, 0x10});
    h.feed({0xB9, 0x04, 0x60});
    const auto &change = h.midi.last_control_change();
    EXPECT_NE(change.sequence, start);
    EXPECT_EQ(change.channel, 9);
    EXPECT_EQ(change.number, 0x04);
    EXPECT_EQ(change.previous, 0x10);
    EXPECT_EQ(change.value, 0x60);
}

TEST(MidiDetection, TracksTheLastPitchBend)
{
    MidiHarness h{false};
    uint16_t start = h.midi.last_pitch_bend().sequence;
    // 14 bits, lsb first
    h.feed({0xE1, 0x00, 0x7F});
    const auto &bend = h.midi.last_pitch_bend();
    EXPECT_NE(bend.sequence, start);
    EXPECT_EQ(bend.channel, 1);
    EXPECT_EQ(bend.previous, MIDI_PITCH_BEND_CENTER);
    EXPECT_EQ(bend.value, 0x7F << 7);
}
