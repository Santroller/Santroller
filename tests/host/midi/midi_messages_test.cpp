// Channel, system common, realtime and sysex messages, run with both framings: a plain MIDI byte stream
// (serial / BLE MIDI) and USB MIDI event packets.
#include <gtest/gtest.h>
#include "midi/midi_test_support.hpp"

using namespace midi_test;

namespace
{
enum class Framing
{
    ByteStream,
    Usb
};

class MidiMessages : public ::testing::TestWithParam<Framing>
{
protected:
    MidiHarness h{GetParam() == Framing::Usb};

    // Send complete messages (each with its own status byte) in one transfer
    void send(std::initializer_list<Bytes> messages)
    {
        Bytes bytes;
        for (const Bytes &message : messages)
        {
            Bytes encoded = GetParam() == Framing::Usb ? usb_message(0, message) : message;
            bytes.insert(bytes.end(), encoded.begin(), encoded.end());
        }
        h.feed(bytes);
    }
};

std::string framing_name(const ::testing::TestParamInfo<Framing> &info)
{
    return info.param == Framing::Usb ? "Usb" : "ByteStream";
}
} // namespace

INSTANTIATE_TEST_SUITE_P(Framings, MidiMessages, ::testing::Values(Framing::ByteStream, Framing::Usb), framing_name);

TEST_P(MidiMessages, NothingReceivedYet)
{
    h.parse();
    EXPECT_TRUE(h.messages.empty());
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_control_change(0, 7), 0);
    for (uint8_t channel = 0; channel < 16; channel++)
    {
        // the wheel rests in the middle of its 14 bit range
        EXPECT_EQ(h.midi.read_midi_pitch_bend(channel), MIDI_PITCH_BEND_CENTER);
    }
    for (uint8_t channel = 0; channel < 18; channel++)
    {
        EXPECT_FALSE(h.midi.has_midi_channel(channel));
    }
}

TEST_P(MidiMessages, NoteOnStoresVelocity)
{
    send({{0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3D), 0);
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3C), 0);
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}}));
}

TEST_P(MidiMessages, ChannelIsTheLowNibbleOfTheStatus)
{
    send({{0x9F, 0x00, 0x01}, {0x95, 0x7F, 0x7F}});
    EXPECT_EQ(h.midi.read_midi_note(15, 0x00), 0x01);
    EXPECT_EQ(h.midi.read_midi_note(5, 0x7F), 0x7F);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x00), 0);
}

TEST_P(MidiMessages, NoteOffClearsVelocity)
{
    send({{0x90, 0x3C, 0x64}, {0x90, 0x3E, 0x50}});
    // note off release velocity doesn't matter
    send({{0x80, 0x3C, 0x40}});
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x50);
}

TEST_P(MidiMessages, NoteOnWithZeroVelocityIsANoteOff)
{
    send({{0x91, 0x3C, 0x64}});
    send({{0x91, 0x3C, 0x00}});
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3C), 0);
    // and it isn't a new note event
    uint16_t sequence = 0, velocity = 0;
    ASSERT_TRUE(h.midi.consume_midi_note_event(1, 0x3C, sequence, velocity));
    EXPECT_EQ(velocity, 0x64 << 9);
    EXPECT_FALSE(h.midi.consume_midi_note_event(1, 0x3C, sequence, velocity));
}

TEST_P(MidiMessages, NoteOffForANoteThatWasNeverOn)
{
    send({{0x82, 0x3C, 0x00}, {0x92, 0x3D, 0x00}});
    EXPECT_EQ(h.midi.read_midi_note(2, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_note(2, 0x3D), 0);
    // still a channel that is in use
    EXPECT_TRUE(h.midi.has_midi_channel(2));
}

TEST_P(MidiMessages, ReadingOutOfRangeNotesAndChannels)
{
    send({{0x90, 0x3C, 0x64}, {0xB0, 0x07, 0x64}});
    EXPECT_EQ(h.midi.read_midi_note(16, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x80), 0);
    EXPECT_EQ(h.midi.read_midi_control_change(16, 0x07), 0);
    EXPECT_EQ(h.midi.read_midi_control_change(0, 0x80), 0);
}

TEST_P(MidiMessages, ControlChangeIsScaledUpTo16Bits)
{
    send({{0xB2, 0x07, 0x64}, {0xB2, 0x40, 0x7F}});
    EXPECT_EQ(h.midi.read_midi_control_change(2, 0x07), 0x64 << 9);
    EXPECT_EQ(h.midi.read_midi_control_change(2, 0x40), 0x7F << 9);
    EXPECT_EQ(h.midi.read_midi_control_change(2, 0x01), 0);
    EXPECT_EQ(h.midi.read_midi_control_change(0, 0x07), 0);
    send({{0xB2, 0x07, 0x00}});
    EXPECT_EQ(h.midi.read_midi_control_change(2, 0x07), 0);
}

// Pitch bend is E0 + channel, then the 7 least significant bits, then the 7 most significant bits of a
// 14 bit value with 0x2000 at rest (MIDI 1.0, Pitch Bend Change).
TEST_P(MidiMessages, PitchBendIsFourteenBitsLsbFirst)
{
    send({{0xE1, 0x00, 0x40}});
    EXPECT_EQ(h.midi.read_midi_pitch_bend(1), 0x2000);
    send({{0xE1, 0x7F, 0x7F}});
    EXPECT_EQ(h.midi.read_midi_pitch_bend(1), 0x3FFF);
    send({{0xE1, 0x00, 0x00}});
    EXPECT_EQ(h.midi.read_midi_pitch_bend(1), 0x0000);
    send({{0xE1, 0x01, 0x02}});
    EXPECT_EQ(h.midi.read_midi_pitch_bend(1), (0x02 << 7) | 0x01);
    // other channels stay where they were
    EXPECT_EQ(h.midi.read_midi_pitch_bend(0), MIDI_PITCH_BEND_CENTER);
}

TEST_P(MidiMessages, PitchBendIsAThreeByteChannelMessage)
{
    // whatever the value is, the message mustn't swallow the note after it
    send({{0xE3, 0x12, 0x34}, {0x93, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xE3, 0x12, 0x34}, {0x93, 0x3C, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(3, 0x3C), 0x64);
    EXPECT_TRUE(h.midi.has_midi_channel(3));
}

TEST_P(MidiMessages, ProgramChangeAndChannelPressureAreTwoBytes)
{
    send({{0xC0, 0x05}, {0xD1, 0x40}, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xC0, 0x05}, {0xD1, 0x40}, {0x90, 0x3C, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_TRUE(h.midi.has_midi_channel(0));
    EXPECT_TRUE(h.midi.has_midi_channel(1));
}

TEST_P(MidiMessages, PolyKeyPressureIsThreeBytesAndDoesntChangeTheNote)
{
    send({{0x90, 0x3C, 0x64}, {0xA0, 0x3C, 0x20}, {0x90, 0x3E, 0x30}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xA0, 0x3C, 0x20}, {0x90, 0x3E, 0x30}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3E), 0x30);
}

TEST_P(MidiMessages, SystemCommonSizes)
{
    // MTC quarter frame and song select have one data byte, song position two, tune request none
    send({{0xF1, 0x12}, {0xF3, 0x05}, {0xF2, 0x10, 0x20}, {0xF6}, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xF1, 0x12}, {0xF3, 0x05}, {0xF2, 0x10, 0x20}, {0xF6}, {0x90, 0x3C, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    // system messages don't belong to a channel
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{0}));
}

TEST_P(MidiMessages, RealtimeMessagesAreSingleBytes)
{
    send({{0xF8}, {0xFA}, {0xFB}, {0xFC}, {0xFE}, {0xFF}, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xF8}, {0xFA}, {0xFB}, {0xFC}, {0xFE}, {0xFF}, {0x90, 0x3C, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{0}));
}

TEST_P(MidiMessages, SysexIsReportedWhole)
{
    Bytes sysex = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
    send({sysex, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{sysex, {0x90, 0x3C, 0x64}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0x64);
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{0}));
}

TEST_P(MidiMessages, EmptySysex)
{
    send({{0xF0, 0xF7}, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0xF0, 0xF7}, {0x90, 0x3C, 0x64}}));
}

TEST_P(MidiMessages, SysexThatFillsTheBuffer)
{
    // 32 bytes including F0 and F7 is the most that is kept
    Bytes sysex = {0xF0};
    for (uint8_t i = 0; i < 30; i++)
    {
        sysex.push_back(i);
    }
    sysex.push_back(0xF7);
    ASSERT_EQ(sysex.size(), 32u);
    send({sysex, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{sysex, {0x90, 0x3C, 0x64}}));
}

// A sysex message that doesn't fit in the 32 byte buffer is dropped rather than decoded cut short
TEST_P(MidiMessages, OverlongSysexIsDropped)
{
    Bytes sysex = {0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45};
    while (sysex.size() < 40)
    {
        sysex.push_back(0x00);
    }
    sysex.push_back(0xF7);
    send({sysex, {0x90, 0x3C, 0x64}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}}));
    EXPECT_EQ(h.midi.pro_guitar_frets()[0], 0);
    EXPECT_FALSE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_MUSTANG));
}

// However long a sysex message is, its data bytes must never be parsed as other messages
TEST_P(MidiMessages, LongSysexPayloadIsNotParsedAsChannelMessages)
{
    Bytes sysex = {0xF0, 0x7D};
    while (sysex.size() < 300)
    {
        sysex.push_back(0x10);
    }
    sysex.push_back(0xF7);
    send({{0x91, 0x3C, 0x64}, sysex, {0x91, 0x3E, 0x64}});
    EXPECT_EQ(h.midi.read_midi_note(1, 0x10), 0);
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3C), 0x64);
    EXPECT_EQ(h.midi.read_midi_note(1, 0x3E), 0x64);
    uint16_t sequence = 0, velocity = 0;
    EXPECT_FALSE(h.midi.peek_midi_note_event(1, 0x10, sequence, velocity));
}

TEST_P(MidiMessages, ChannelMessagesMarkTheirChannel)
{
    send({{0x93, 0x3C, 0x64}, {0x83, 0x3C, 0x00}, {0xB7, 0x07, 0x00}, {0xEA, 0x00, 0x40}, {0xCC, 0x01}, {0xDD, 0x01}, {0xAE, 0x3C, 0x01}});
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{3, 3, 7, 10, 12, 13, 14}));
    EXPECT_EQ(h.newly_seen, (std::vector<uint8_t>{3, 7, 10, 12, 13, 14}));
    for (uint8_t channel : {3, 7, 10, 12, 13, 14})
    {
        EXPECT_TRUE(h.midi.has_midi_channel(channel)) << int(channel);
    }
    EXPECT_FALSE(h.midi.has_midi_channel(0));
    EXPECT_FALSE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_SQUIER));
    EXPECT_FALSE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_MUSTANG));
}

TEST_P(MidiMessages, SeenChannelsSurviveAReload)
{
    send({{0x92, 0x3C, 0x64}, {0x9F, 0x3C, 0x64}});
    bool saved[18] = {};
    h.midi.save_seen_channels(saved);
    MidiHarness restored(GetParam() == Framing::Usb);
    restored.midi.load_seen_channels(saved);
    EXPECT_TRUE(restored.midi.has_midi_channel(2));
    EXPECT_TRUE(restored.midi.has_midi_channel(15));
    EXPECT_FALSE(restored.midi.has_midi_channel(0));
    EXPECT_FALSE(restored.midi.set_channel_seen(2));
    EXPECT_TRUE(restored.midi.set_channel_seen(0));
}

TEST_P(MidiMessages, OneMessagePerTransfer)
{
    // each transfer is parsed by its own update()
    send({{0x90, 0x3C, 0x64}});
    send({{0xB0, 0x07, 0x50}});
    send({{0xF0, 0x01, 0x02, 0xF7}});
    send({{0x80, 0x3C, 0x00}});
    EXPECT_EQ(h.messages, (std::vector<Bytes>{{0x90, 0x3C, 0x64}, {0xB0, 0x07, 0x50}, {0xF0, 0x01, 0x02, 0xF7}, {0x80, 0x3C, 0x00}}));
    EXPECT_EQ(h.midi.read_midi_note(0, 0x3C), 0);
    EXPECT_EQ(h.midi.read_midi_control_change(0, 0x07), 0x50 << 9);
    EXPECT_EQ(h.stream.available(), 0u);
}
