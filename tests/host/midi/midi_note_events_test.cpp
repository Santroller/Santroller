// The note on event queue: each note on is queued with a sequence number so that inputs see every hit,
// even when a note goes on and off again between two polls (e.g. a drum hit).
#include <gtest/gtest.h>
#include "midi/midi_test_support.hpp"

using namespace midi_test;

namespace
{
class MidiNoteEvents : public ::testing::Test
{
protected:
    MidiHarness h{false};

    void note_on(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        h.feed({static_cast<uint8_t>(0x90 | channel), note, velocity});
    }
};
} // namespace

TEST_F(MidiNoteEvents, NoEventsYet)
{
    uint16_t sequence = 0, velocity = 0xFFFF;
    EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x3C, sequence, velocity));
    EXPECT_FALSE(h.midi.peek_midi_note_event(0, 0x3C, sequence, velocity));
    EXPECT_EQ(sequence, 0);
    EXPECT_EQ(velocity, 0xFFFF);
}

TEST_F(MidiNoteEvents, VelocityIsScaledUpTo16Bits)
{
    note_on(0, 0x3C, 0x7F);
    uint16_t sequence = 0, velocity = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x3C, sequence, velocity));
    EXPECT_EQ(velocity, 0x7F << 9);
}

TEST_F(MidiNoteEvents, ConsumeReturnsEachHitOldestFirst)
{
    // three hits on the same note before anything reads them, released in between
    note_on(0, 0x26, 0x10);
    h.feed({0x80, 0x26, 0x00});
    note_on(0, 0x26, 0x20);
    h.feed({0x90, 0x26, 0x00});
    note_on(0, 0x26, 0x30);
    uint16_t sequence = 0, velocity = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    EXPECT_EQ(velocity, 0x10 << 9);
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    EXPECT_EQ(velocity, 0x20 << 9);
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    EXPECT_EQ(velocity, 0x30 << 9);
    EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    // a new hit shows up after that
    note_on(0, 0x26, 0x40);
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    EXPECT_EQ(velocity, 0x40 << 9);
}

TEST_F(MidiNoteEvents, SequenceNumbersIncrease)
{
    note_on(0, 0x26, 0x10);
    note_on(0, 0x26, 0x20);
    uint16_t sequence = 0, velocity = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    uint16_t first = sequence;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
    EXPECT_GT(sequence, first);
}

TEST_F(MidiNoteEvents, EachReaderKeepsItsOwnPlace)
{
    note_on(0, 0x26, 0x10);
    note_on(0, 0x26, 0x20);
    uint16_t a = 0, b = 0, velocity = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, a, velocity));
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, a, velocity));
    // consuming doesn't remove anything, b still sees both hits
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, b, velocity));
    EXPECT_EQ(velocity, 0x10 << 9);
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, b, velocity));
    EXPECT_EQ(velocity, 0x20 << 9);
    EXPECT_EQ(a, b);
}

TEST_F(MidiNoteEvents, EventsAreForOneChannelAndNote)
{
    note_on(0, 0x26, 0x10);
    note_on(1, 0x26, 0x20);
    note_on(0, 0x27, 0x30);
    uint16_t sequence = 0, velocity = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(1, 0x26, sequence, velocity));
    EXPECT_EQ(velocity, 0x20 << 9);
    EXPECT_FALSE(h.midi.consume_midi_note_event(1, 0x26, sequence, velocity));
    sequence = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x27, sequence, velocity));
    EXPECT_EQ(velocity, 0x30 << 9);
    EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x27, sequence, velocity));
    sequence = 0;
    EXPECT_FALSE(h.midi.consume_midi_note_event(2, 0x26, sequence, velocity));
}

TEST_F(MidiNoteEvents, OnlyNoteOnsAreQueued)
{
    h.feed({0x80, 0x26, 0x40, 0x90, 0x26, 0x00, 0xB0, 0x26, 0x40, 0xA0, 0x26, 0x40});
    uint16_t sequence = 0, velocity = 0;
    EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
}

TEST_F(MidiNoteEvents, PeekReturnsTheNewestHit)
{
    note_on(0, 0x26, 0x10);
    note_on(0, 0x26, 0x20);
    note_on(0, 0x27, 0x30);
    uint16_t sequence = 0, velocity = 0;
    ASSERT_TRUE(h.midi.peek_midi_note_event(0, 0x26, sequence, velocity));
    EXPECT_EQ(velocity, 0x20 << 9);
    // nothing newer than that
    EXPECT_FALSE(h.midi.peek_midi_note_event(0, 0x26, sequence, velocity));
    // peeking doesn't affect consumers
    uint16_t consumed = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, consumed, velocity));
    EXPECT_EQ(velocity, 0x10 << 9);
}

TEST_F(MidiNoteEvents, FullQueueDropsTheOldestEvent)
{
    const size_t capacity = MidiInput::MIDI_NOTE_EVENT_CAPACITY;
    // one more hit than fits
    for (size_t i = 0; i <= capacity; i++)
    {
        note_on(0, 0x26, static_cast<uint8_t>(i + 1));
    }
    uint16_t sequence = 0, velocity = 0;
    for (size_t i = 1; i <= capacity; i++)
    {
        ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity)) << i;
        EXPECT_EQ(velocity, (i + 1) << 9) << i;
    }
    EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity));
}

TEST_F(MidiNoteEvents, FullQueueForgetsHitsOnOtherNotes)
{
    note_on(0, 0x24, 0x7F);
    for (size_t i = 0; i < MidiInput::MIDI_NOTE_EVENT_CAPACITY; i++)
    {
        note_on(0, 0x26, 0x40);
    }
    uint16_t sequence = 0, velocity = 0;
    EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x24, sequence, velocity));
    EXPECT_FALSE(h.midi.peek_midi_note_event(0, 0x24, sequence, velocity));
    // the note itself is still held
    EXPECT_EQ(h.midi.read_midi_note(0, 0x24), 0x7F);
}

TEST_F(MidiNoteEvents, QueueWrapsAroundRepeatedly)
{
    // keep a reader up to date while the ring buffer goes round many times
    uint16_t sequence = 0, velocity = 0;
    for (int i = 0; i < 1000; i++)
    {
        uint8_t hit = static_cast<uint8_t>(i % 127 + 1);
        note_on(0, 0x26, hit);
        ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity)) << i;
        EXPECT_EQ(velocity, hit << 9) << i;
        EXPECT_FALSE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity)) << i;
    }
}

TEST_F(MidiNoteEvents, SequenceNumbersWrapAround)
{
    // the 16 bit sequence wraps after 65536 hits; a reader that keeps up never notices
    uint16_t sequence = 0, velocity = 0;
    h.feed({0x90});
    for (int i = 0; i < 70000; i++)
    {
        uint8_t hit = static_cast<uint8_t>(i % 127 + 1);
        // running status keeps this quick
        h.feed({0x26, hit});
        ASSERT_TRUE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity)) << i;
        ASSERT_EQ(velocity, hit << 9) << i;
        ASSERT_FALSE(h.midi.consume_midi_note_event(0, 0x26, sequence, velocity)) << i;
    }
}
