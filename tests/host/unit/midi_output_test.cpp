#include <gtest/gtest.h>
#include <vector>
#include "protocols/midi_output.hpp"

namespace
{
using Messages = std::vector<MidiMessage>;

// Everything pending, as if the transport sent it all straight away
Messages drain(MidiOutput &output)
{
    Messages out;
    MidiMessage message;
    while (output.pending(message))
    {
        out.push_back(message);
        output.sent(message);
    }
    return out;
}

MidiState notes(std::initializer_list<MidiState::Note> held)
{
    MidiState state;
    for (const auto &note : held)
    {
        state.set_note(note.channel, note.note, note.velocity);
    }
    return state;
}

constexpr uint8_t SNARE = 38;
constexpr uint8_t KICK = 36;
} // namespace

TEST(MidiOutput, UsbPacketUsesTheStatusAsTheCodeIndex)
{
    uint8_t packet[4];
    midi_usb_packet({0x93, 0x3C, 0x40}, 0, packet);
    EXPECT_EQ(packet[0], 0x09);
    EXPECT_EQ(packet[1], 0x93);
    EXPECT_EQ(packet[2], 0x3C);
    EXPECT_EQ(packet[3], 0x40);
    midi_usb_packet({0xE0, 0x00, 0x40}, 1, packet);
    EXPECT_EQ(packet[0], 0x1E);
}

TEST(MidiOutput, DigitalHitGoesOutStraightAway)
{
    MidiOutput output;
    output.collect(notes({{9, SNARE, 127}}), 0);
    EXPECT_EQ(drain(output), (Messages{{0x99, SNARE, 127}}));
    // held, nothing new
    output.collect(notes({{9, SNARE, 127}}), 1000);
    EXPECT_EQ(drain(output), Messages{});
    output.collect(notes({}), 2000);
    EXPECT_EQ(drain(output), (Messages{{0x89, SNARE, 0}}));
    output.collect(notes({}), 3000);
    EXPECT_EQ(drain(output), Messages{});
}

TEST(MidiOutput, AnalogHitWaitsForItsPeak)
{
    MidiOutput output;
    output.collect(notes({{9, SNARE, 20}}), 0);
    EXPECT_EQ(drain(output), Messages{});
    output.collect(notes({{9, SNARE, 90}}), 500);
    EXPECT_EQ(drain(output), Messages{});
    output.collect(notes({{9, SNARE, 60}}), 1000);
    EXPECT_EQ(drain(output), Messages{});
    output.collect(notes({{9, SNARE, 40}}), MidiOutput::SCAN_US);
    EXPECT_EQ(drain(output), (Messages{{0x99, SNARE, 90}}));
    // later readings don't change a note that's already sounding
    output.collect(notes({{9, SNARE, 127}}), MidiOutput::SCAN_US + 500);
    EXPECT_EQ(drain(output), Messages{});
}

TEST(MidiOutput, HitShorterThanTheScanStillSounds)
{
    MidiOutput output;
    output.collect(notes({{9, KICK, 50}}), 0);
    output.collect(notes({{9, KICK, 70}}), 300);
    EXPECT_EQ(drain(output), Messages{});
    output.collect(notes({}), 600);
    EXPECT_EQ(drain(output), (Messages{{0x99, KICK, 70}, {0x89, KICK, 0}}));
}

TEST(MidiOutput, HitAgainBeforeTheNoteOffWentOut)
{
    MidiOutput output;
    output.collect(notes({{9, SNARE, 127}}), 0);
    EXPECT_EQ(drain(output), (Messages{{0x99, SNARE, 127}}));
    // released and hit again before the transport took the note off
    output.collect(notes({}), 1000);
    output.collect(notes({{9, SNARE, 127}}), 2000);
    EXPECT_EQ(drain(output), (Messages{{0x89, SNARE, 0}, {0x99, SNARE, 127}}));
}

TEST(MidiOutput, BusyTransportLosesNothing)
{
    MidiOutput output;
    output.collect(notes({{9, SNARE, 127}, {9, KICK, 127}}), 0);
    MidiMessage message;
    ASSERT_TRUE(output.pending(message));
    output.sent(message);
    // the second note on couldn't go out before both were released
    output.collect(notes({}), 1000);
    Messages rest = drain(output);
    EXPECT_EQ(rest.size(), 3u);
    // the note that sounded is released first, then the other one sounds and is released
    EXPECT_EQ(rest[0].status, 0x89);
    EXPECT_EQ(rest[0].data1, message.data1);
    EXPECT_EQ(rest[1].status, 0x99);
    EXPECT_EQ(rest[2].status, 0x89);
    EXPECT_EQ(rest[1].data1, rest[2].data1);
    EXPECT_NE(rest[1].data1, message.data1);
}

TEST(MidiOutput, SharedNoteTakesTheHardestHit)
{
    MidiState state;
    state.set_note(0, 60, 30);
    state.set_note(0, 60, 100);
    state.set_note(0, 60, 50);
    ASSERT_EQ(state.note_count, 1);
    EXPECT_EQ(state.notes[0].velocity, 100);
}

TEST(MidiOutput, ControlChangesOnlyWhenTheValueDoes)
{
    MidiOutput output;
    MidiState state;
    state.set_control(0, 4, 0);
    output.collect(state, 0);
    EXPECT_EQ(drain(output), (Messages{{0xB0, 4, 0}}));
    output.collect(state, 1000);
    EXPECT_EQ(drain(output), Messages{});
    state.clear_all();
    state.set_control(0, 4, 90);
    output.collect(state, 2000);
    EXPECT_EQ(drain(output), (Messages{{0xB0, 4, 90}}));
}

TEST(MidiOutput, PitchBendIsLeastSignificantSevenBitsFirst)
{
    MidiOutput output;
    MidiState state;
    state.set_pitch_bend(2, 0x2000);
    output.collect(state, 0);
    EXPECT_EQ(drain(output), (Messages{{0xE2, 0x00, 0x40}}));
    state.clear_all();
    state.set_pitch_bend(2, 0x3FFF);
    output.collect(state, 1000);
    EXPECT_EQ(drain(output), (Messages{{0xE2, 0x7F, 0x7F}}));
}

TEST(MidiOutput, SharedPitchBendTakesTheFurthestFromCentre)
{
    MidiState state;
    state.set_pitch_bend(0, 0x2100);
    state.set_pitch_bend(0, 0x1000);
    state.set_pitch_bend(0, 0x2000);
    EXPECT_EQ(state.pitch_bend[0], 0x1000);
}

TEST(MidiOutput, MergeCombinesProfiles)
{
    MidiState a = notes({{9, SNARE, 40}});
    a.set_control(0, 4, 10);
    MidiState b = notes({{9, SNARE, 80}, {9, KICK, 127}});
    b.set_control(0, 4, 20);
    b.set_pitch_bend(1, 0x3000);
    a.merge(b);
    ASSERT_EQ(a.note_count, 2);
    EXPECT_EQ(a.notes[0].velocity, 80);
    ASSERT_EQ(a.control_count, 1);
    EXPECT_EQ(a.controls[0].value, 20);
    EXPECT_EQ(a.pitch_bend_mask, 1u << 1);
    EXPECT_EQ(a.pitch_bend[1], 0x3000);
}

TEST(MidiScale, EndsAndCentreAreExact)
{
    EXPECT_EQ(midi_scale(0, 50, 7), 0);
    EXPECT_EQ(midi_scale(UINT16_MAX, 50, 7), 127);
    EXPECT_EQ(midi_scale(UINT16_MAX / 2, 50, 7), 64);
    EXPECT_EQ(midi_scale(UINT16_MAX / 2, 100, 14), 0x2000);
    EXPECT_EQ(midi_scale(UINT16_MAX, 100, 14), 0x3FFF);
}

TEST(MidiScale, IgnoresJitterAroundTheLastValue)
{
    // 50 covers 25600 - 26111 at 7 bits, and a step is 512 wide so the margin is 128
    EXPECT_EQ(midi_scale(26112, 50, 7), 50);
    EXPECT_EQ(midi_scale(26239, 50, 7), 50);
    EXPECT_EQ(midi_scale(26240, 50, 7), 51);
    EXPECT_EQ(midi_scale(25472, 50, 7), 50);
    EXPECT_EQ(midi_scale(25471, 50, 7), 49);
    // at 14 bits a step is only 4 wide, so the margin is the minimum of 64
    EXPECT_EQ(midi_scale(1000 * 4 + 67, 1000, 14), 1000);
    EXPECT_EQ(midi_scale(1000 * 4 + 68, 1000, 14), 1017);
}

TEST(MidiScale, VelocityIsNeverZero)
{
    EXPECT_EQ(midi_velocity(0), 1);
    EXPECT_EQ(midi_velocity(511), 1);
    EXPECT_EQ(midi_velocity(1024), 2);
    EXPECT_EQ(midi_velocity(UINT16_MAX), 127);
}

TEST(MidiOutput, HitTheHostNeverTookIsDropped)
{
    MidiOutput output;
    output.collect(notes({{9, SNARE, 127}}), 0);
    output.collect(notes({}), 1000);
    // the transport stayed busy the whole time
    output.collect(notes({}), 1000 + MidiOutput::STALE_US);
    MidiMessage message;
    ASSERT_TRUE(output.pending(message));
    output.collect(notes({}), 1001 + MidiOutput::STALE_US);
    EXPECT_FALSE(output.pending(message));
}

TEST(MidiOutput, SoundingNoteIsNeverDropped)
{
    MidiOutput output;
    output.collect(notes({{9, SNARE, 127}}), 0);
    EXPECT_EQ(drain(output), (Messages{{0x99, SNARE, 127}}));
    output.collect(notes({}), 1000);
    output.collect(notes({}), 1001 + MidiOutput::STALE_US);
    EXPECT_EQ(drain(output), (Messages{{0x89, SNARE, 0}}));
}
