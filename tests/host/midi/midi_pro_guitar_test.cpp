// Rock Band 3 Pro Guitar (Squier and Mustang) sysex messages:
//   F0 08 40 <id> <type> ... F7
// id is 08 for the Squier and 0A for the Mustang. type 01 is a fret change (string number 1 - 6, then the
// note that string now plays), type 05 a pick (string number, velocity) and type 08 the buttons, laid out
// as ProGuitar_Sysex_Buttons_t.
#include <gtest/gtest.h>
#include "midi/midi_test_support.hpp"

using namespace midi_test;

namespace
{
// open string notes in standard tuning, string 1 (high E, E4) to string 6 (low E, E2)
const uint8_t kOpenStrings[6] = {64, 59, 55, 50, 45, 40};

class MidiProGuitar : public ::testing::TestWithParam<bool>
{
protected:
    MidiHarness h{GetParam()};

    void send(const Bytes &sysex)
    {
        h.feed(GetParam() ? usb_message(0, sysex) : sysex);
    }
    void fret(uint8_t string, uint8_t note)
    {
        send({0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, string, note, 0xF7});
    }
    void pick(uint8_t string, uint8_t velocity)
    {
        send({0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x05, string, velocity, 0xF7});
    }
    void buttons(uint8_t b0, uint8_t b1, uint8_t b2)
    {
        send({0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x08, b0, b1, b2, 0xF7});
    }
};

std::string framing_name(const ::testing::TestParamInfo<bool> &info)
{
    return info.param ? "Usb" : "ByteStream";
}
} // namespace

INSTANTIATE_TEST_SUITE_P(Framings, MidiProGuitar, ::testing::Bool(), framing_name);

TEST_P(MidiProGuitar, StartsNeutral)
{
    for (int string = 0; string < 6; string++)
    {
        EXPECT_EQ(h.midi.pro_guitar_frets()[string], 0);
        EXPECT_EQ(h.midi.pro_guitar_string_velocities()[string], 0);
    }
    const ProGuitar_Sysex_Buttons_t &b = h.midi.pro_guitar_buttons();
    EXPECT_FALSE(b.a || b.b || b.x || b.y || b.back || b.start || b.guide || b.tilt);
    // 8 is the dpad's centre
    EXPECT_EQ(b.dpad, 8);
}

TEST_P(MidiProGuitar, SquierMarksItsChannel)
{
    send({0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_SQUIER, 0x01, 0x01, 0x40, 0xF7});
    EXPECT_TRUE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_SQUIER));
    EXPECT_FALSE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_MUSTANG));
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{MIDI_CHANNEL_PROGUITAR_SQUIER}));
    // only marked the first time
    send({0xF0, 0x08, 0x40, MIDI_SYSEX_ID_PROGUITAR_SQUIER, 0x01, 0x01, 0x41, 0xF7});
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{MIDI_CHANNEL_PROGUITAR_SQUIER}));
}

TEST_P(MidiProGuitar, MustangMarksItsChannel)
{
    fret(1, 0x40);
    EXPECT_TRUE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_MUSTANG));
    EXPECT_FALSE(h.midi.has_midi_channel(MIDI_CHANNEL_PROGUITAR_SQUIER));
    EXPECT_EQ(h.marked, (std::vector<uint8_t>{MIDI_CHANNEL_PROGUITAR_MUSTANG}));
    // sysex doesn't mark a regular channel
    EXPECT_FALSE(h.midi.has_midi_channel(0));
}

TEST_P(MidiProGuitar, OtherSysexIsIgnored)
{
    // different manufacturer / device bytes
    send({0xF0, 0x08, 0x41, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7});
    send({0xF0, 0x09, 0x40, MIDI_SYSEX_ID_PROGUITAR_MUSTANG, 0x01, 0x01, 0x45, 0xF7});
    send({0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7});
    EXPECT_EQ(h.midi.pro_guitar_frets()[0], 0);
    EXPECT_TRUE(h.marked.empty());
}

TEST_P(MidiProGuitar, FretsAreTheNoteAboveTheOpenString)
{
    for (uint8_t string = 1; string <= 6; string++)
    {
        uint8_t open = kOpenStrings[string - 1];
        fret(string, open);
        EXPECT_EQ(h.midi.pro_guitar_frets()[string - 1], 0) << int(string);
        fret(string, open + 7);
        EXPECT_EQ(h.midi.pro_guitar_frets()[string - 1], 7) << int(string);
        // 22 frets on the Mustang
        fret(string, open + 22);
        EXPECT_EQ(h.midi.pro_guitar_frets()[string - 1], 22) << int(string);
    }
}

TEST_P(MidiProGuitar, FretsAreIndependentPerString)
{
    for (uint8_t string = 1; string <= 6; string++)
    {
        fret(string, kOpenStrings[string - 1] + string);
    }
    for (uint8_t string = 1; string <= 6; string++)
    {
        EXPECT_EQ(h.midi.pro_guitar_frets()[string - 1], string);
    }
}

TEST_P(MidiProGuitar, FretsForUnknownStringsAreIgnored)
{
    fret(0, 0x45);
    fret(7, 0x45);
    for (int string = 0; string < 6; string++)
    {
        EXPECT_EQ(h.midi.pro_guitar_frets()[string], 0);
    }
}

TEST_P(MidiProGuitar, PicksSetStringVelocity)
{
    pick(1, 0x40);
    pick(6, 0x7F);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[0], 0x40);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[5], 0x7F);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[2], 0);
}

TEST_P(MidiProGuitar, RepeatedPicksStillChangeTheVelocity)
{
    // picking again at the same velocity flips the lowest bit so the new pick is visible
    pick(3, 0x40);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[2], 0x40);
    pick(3, 0x40);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[2], 0x41);
    pick(3, 0x40);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[2], 0x40);
    // a different velocity is kept as it is
    pick(3, 0x7F);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[2], 0x7F);
    pick(3, 0x7F);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[2], 0x7E);
}

// Like frets, picks only exist for strings 1 - 6 (string 0 would otherwise be index 255)
TEST_P(MidiProGuitar, PicksForUnknownStringsAreIgnored)
{
    pick(7, 0x40);
    pick(0, 0x40);
    for (int string = 0; string < 6; string++)
    {
        EXPECT_EQ(h.midi.pro_guitar_string_velocities()[string], 0);
    }
}

TEST_P(MidiProGuitar, Buttons)
{
    // x a b y in the first byte, back start . . guide in the second, the dpad hat and tilt in the third
    buttons(0x02, 0x00, 0x08);
    {
        const ProGuitar_Sysex_Buttons_t &b = h.midi.pro_guitar_buttons();
        EXPECT_TRUE(b.a);
        EXPECT_FALSE(b.x || b.b || b.y || b.back || b.start || b.guide || b.tilt);
        EXPECT_EQ(b.dpad, 8);
    }
    buttons(0x0D, 0x13, 0x42);
    {
        const ProGuitar_Sysex_Buttons_t &b = h.midi.pro_guitar_buttons();
        EXPECT_TRUE(b.x);
        EXPECT_FALSE(b.a);
        EXPECT_TRUE(b.b);
        EXPECT_TRUE(b.y);
        EXPECT_TRUE(b.back);
        EXPECT_TRUE(b.start);
        EXPECT_TRUE(b.guide);
        EXPECT_TRUE(b.tilt);
        EXPECT_EQ(b.dpad, 2);
    }
    buttons(0x00, 0x00, 0x08);
    {
        const ProGuitar_Sysex_Buttons_t &b = h.midi.pro_guitar_buttons();
        EXPECT_FALSE(b.a || b.b || b.x || b.y || b.back || b.start || b.guide || b.tilt);
        EXPECT_EQ(b.dpad, 8);
    }
}

TEST_P(MidiProGuitar, ButtonsDontChangeFrets)
{
    fret(2, kOpenStrings[1] + 3);
    pick(2, 0x30);
    buttons(0x0F, 0x13, 0x40);
    EXPECT_EQ(h.midi.pro_guitar_frets()[1], 3);
    EXPECT_EQ(h.midi.pro_guitar_string_velocities()[1], 0x30);
}

TEST_P(MidiProGuitar, SustainPedalIsAControlChange)
{
    // the pedal is sent as the sustain pedal CC on channel 1
    h.feed(GetParam() ? usb_message(0, {0xB0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL, 0x7F}) : Bytes{0xB0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL, 0x7F});
    EXPECT_EQ(h.midi.read_midi_control_change(0, MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL), 0x7F << 9);
}

// The axes and the console reports go low E first (see PlasticBand's Pro Guitar docs), while the sysex
// numbers strings from high E, so each named axis has to come from the opposite end
TEST_P(MidiProGuitar, FretAxesFollowTheirStrings)
{
    const ProGuitarAxisType axes[6] = {ProGuitar_HighEFret, ProGuitar_BFret, ProGuitar_GFret,
                                       ProGuitar_DFret, ProGuitar_AFret, ProGuitar_LowEFret};
    for (uint8_t string = 1; string <= 6; string++)
    {
        // a different fret on each string, so a mixed up string can't pass by accident
        fret(string, kOpenStrings[string - 1] + string);
    }
    for (uint8_t string = 1; string <= 6; string++)
    {
        EXPECT_EQ(h.midi.pro_guitar_axis(axes[string - 1]), string) << "string " << int(string);
    }
}

TEST_P(MidiProGuitar, LowEFretIsTheLowEString)
{
    // fifth fret on the low E string (E2 + 5 = A2)
    fret(6, 45);
    EXPECT_EQ(h.midi.pro_guitar_axis(ProGuitar_LowEFret), 5);
    EXPECT_EQ(h.midi.pro_guitar_axis(ProGuitar_HighEFret), 0);
}

TEST_P(MidiProGuitar, VelocityAxesFollowTheirStrings)
{
    const ProGuitarAxisType axes[6] = {ProGuitar_HighEFretVelocity, ProGuitar_BFretVelocity, ProGuitar_GFretVelocity,
                                       ProGuitar_DFretVelocity, ProGuitar_AFretVelocity, ProGuitar_LowEFretVelocity};
    for (uint8_t string = 1; string <= 6; string++)
    {
        pick(string, 0x10 * string);
    }
    for (uint8_t string = 1; string <= 6; string++)
    {
        // velocities scale up to the top byte of the axis
        EXPECT_EQ(h.midi.pro_guitar_axis(axes[string - 1]), (0x10 * string) << 8) << "string " << int(string);
    }
}

TEST_P(MidiProGuitar, TiltAxis)
{
    EXPECT_EQ(h.midi.pro_guitar_axis(ProGuitar_Tilt), 32767);
}
