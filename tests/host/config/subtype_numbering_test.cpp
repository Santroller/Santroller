// SubType numbering (subtype_numbering.hpp): every subtype is valid BCD as it is, the old numbering's values only
// ever mean their replacements, and things derived from the old values (slot order, bluetooth addresses) stay put.
#include "subtype_numbering.hpp"
#include "config/slot_ids.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

namespace
{
// SubType's values, read from enums.proto, so a subtype added there is checked without listing it here
std::map<std::string, uint32_t> proto_subtypes()
{
    std::ifstream file(SANTROLLER_PROTO_DIR "/enums.proto");
    std::stringstream text;
    text << file.rdbuf();
    std::string proto = text.str();
    std::map<std::string, uint32_t> values;
    size_t start = proto.find("enum SubType {");
    size_t end = proto.find('}', start);
    if (start == std::string::npos || end == std::string::npos)
    {
        return values;
    }
    std::string body = proto.substr(start, end - start);
    std::regex entry(R"((\w+)\s*=\s*(\d+)\s*;)");
    for (std::sregex_iterator it(body.begin(), body.end(), entry), last; it != last; ++it)
    {
        values[(*it)[1]] = std::stoul((*it)[2]);
    }
    return values;
}
} // namespace

TEST(SubTypeNumbering, EverySubtypeIsBcd)
{
    auto subtypes = proto_subtypes();
    ASSERT_GT(subtypes.size(), 20u);
    for (const auto &[name, value] : subtypes)
    {
        if (name == "Unknown")
        {
            continue;
        }
        EXPECT_TRUE(subtype_is_bcd(value)) << name << " = " << value << " has a hex digit above 9";
    }
}

TEST(SubTypeNumbering, OldValuesAreNeverReused)
{
    for (const auto &[name, value] : proto_subtypes())
    {
        for (const auto &entry : SUBTYPE_RENUMBERING)
        {
            EXPECT_NE(value, entry.old_value) << name << " reuses an old value, which old configs mean something else by";
        }
    }
}

TEST(SubTypeNumbering, CurrentValuesAreLeftAlone)
{
    for (const auto &[name, value] : proto_subtypes())
    {
        EXPECT_EQ(subtype_from_stored(value), (SubType)value) << name;
    }
}

TEST(SubTypeNumbering, OldValuesMapToTheirReplacements)
{
    EXPECT_EQ(subtype_from_stored(10), DisneyInfinity);
    EXPECT_EQ(subtype_from_stored(13), ProjectDiva);
    EXPECT_EQ(subtype_from_stored(15), Wheel);
    EXPECT_EQ(subtype_from_stored(28), Midi);
    EXPECT_EQ(subtype_from_stored(31), KeyboardMania);
    for (const auto &entry : SUBTYPE_RENUMBERING)
    {
        EXPECT_EQ(subtype_old_value(entry.subtype), entry.old_value);
        EXPECT_EQ(subtype_from_stored(subtype_old_value(entry.subtype)), entry.subtype);
    }
}

TEST(SubTypeNumbering, MatchesSantroller1)
{
    // Santroller 1's values for the subtypes both have
    EXPECT_EQ(StageKit, 9);
    EXPECT_EQ(ProKeys, 0x10);
    EXPECT_EQ(ProGuitarMustang, 0x11);
    EXPECT_EQ(ProGuitarSquier, 0x12);
    EXPECT_EQ(Taiko, 0x18);
}

TEST(SubTypeNumbering, SlotsKeepTheirOrder)
{
    // Slots are numbered in order of type, so a renumbered subtype keeps its place: Wheel was 15, before ProKeys
    EXPECT_LT(slot_subtype(Wheel), slot_subtype(ProKeys));
    EXPECT_LT(slot_subtype(KeyboardMania), slot_subtype(Microphone));
    // and an old config's value gives the same type as the new one
    EXPECT_EQ(slot_subtype((SubType)15), slot_subtype(Wheel));
}
