// decode_profile_opts: ProfileOpts as config.cpp's load_opts decodes it, cutting an over long name short
// instead of failing the profile
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <pb_encode.h>
#include "config/profile_opts.hpp"

namespace
{
// A ProfileOpts encoded by hand, so the name can be longer than nanopb's struct holds
std::vector<uint8_t> opts_bytes(const std::string &name, bool extra_fields = true)
{
    std::vector<uint8_t> out = {0x08, 0x2A,  // uid = 42
                                0x10, 0x02}; // faceButtonMappingMode = PositionBased
    out.push_back(0x2A);                     // name
    out.push_back(uint8_t(name.size()));
    out.insert(out.end(), name.begin(), name.end());
    out.insert(out.end(), {0x30, 0x03}); // deviceToEmulate = GuitarHeroGuitar
    if (extra_fields)
    {
        out.insert(out.end(), {0x80, 0x01, 0x01,   // queueInputs = true
                               0x88, 0x01, 0x14}); // dequeueInterval100us = 20
    }
    return out;
}

bool decode(const std::vector<uint8_t> &bytes, proto_ProfileOpts &opts)
{
    opts = proto_ProfileOpts_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    return decode_profile_opts(&stream, &opts);
}
}

TEST(ProfileOptsTest, AnOrdinaryNameDecodesLikeNanopb)
{
    const auto bytes = opts_bytes("Guitar");
    proto_ProfileOpts expected = proto_ProfileOpts_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    ASSERT_TRUE(pb_decode_ex(&stream, proto_ProfileOpts_fields, &expected, PB_DECODE_NOINIT));

    proto_ProfileOpts opts;
    ASSERT_TRUE(decode(bytes, opts));
    EXPECT_EQ(opts.uid, 42u);
    EXPECT_EQ(opts.faceButtonMappingMode, PositionBased);
    EXPECT_STREQ(opts.name, "Guitar");
    EXPECT_EQ(opts.deviceToEmulate, GuitarHeroGuitar);
    EXPECT_TRUE(opts.has_queueInputs && opts.queueInputs);
    EXPECT_EQ(opts.dequeueInterval100us, 20u);
    EXPECT_EQ(opts.uid, expected.uid);
    EXPECT_STREQ(opts.name, expected.name);
    EXPECT_EQ(opts.has_dequeueInterval100us, expected.has_dequeueInterval100us);
    EXPECT_EQ(opts.has_ps3OnRpcs3, expected.has_ps3OnRpcs3);
}

TEST(ProfileOptsTest, ANameOfExactly31BytesIsKept)
{
    const std::string name(31, 'n');
    proto_ProfileOpts opts;
    ASSERT_TRUE(decode(opts_bytes(name), opts));
    EXPECT_EQ(std::string(opts.name), name);
}

TEST(ProfileOptsTest, ALongerNameIsCutTo31BytesAndTheRestStillDecodes)
{
    proto_ProfileOpts opts;
    ASSERT_TRUE(decode(opts_bytes(std::string(32, 'a') + "bcdefgh"), opts));
    EXPECT_EQ(std::string(opts.name), std::string(31, 'a'));
    EXPECT_EQ(opts.uid, 42u);
    EXPECT_EQ(opts.deviceToEmulate, GuitarHeroGuitar);
    EXPECT_TRUE(opts.has_queueInputs && opts.queueInputs);
    EXPECT_EQ(opts.dequeueInterval100us, 20u);
}

TEST(ProfileOptsTest, ANameIsNotCutHalfwayThroughAUtf8Character)
{
    // 30 ASCII bytes then "é" (2 bytes): byte 31 is the first half of the é
    proto_ProfileOpts opts;
    ASSERT_TRUE(decode(opts_bytes(std::string(30, 'x') + "\xC3\xA9" + "z"), opts));
    EXPECT_EQ(std::string(opts.name), std::string(30, 'x'));

    // 28 ASCII bytes then a 4 byte emoji, which ends exactly on byte 32
    ASSERT_TRUE(decode(opts_bytes(std::string(28, 'x') + "\xF0\x9F\x8E\xB8"), opts));
    EXPECT_EQ(std::string(opts.name), std::string(28, 'x'));

    // 27 ASCII bytes then the emoji fits in 31 bytes, so it stays even with more after it
    ASSERT_TRUE(decode(opts_bytes(std::string(27, 'x') + "\xF0\x9F\x8E\xB8" + "yy"), opts));
    EXPECT_EQ(std::string(opts.name), std::string(27, 'x') + "\xF0\x9F\x8E\xB8");
}

TEST(ProfileOptsTest, UnknownFieldsAreSkipped)
{
    auto bytes = opts_bytes("Pad", false);
    bytes.insert(bytes.end(), {0xF8, 0x07, 0x05,       // field 127, varint
                               0xF2, 0x07, 0x02, 1, 2}); // field 126, bytes
    proto_ProfileOpts opts;
    ASSERT_TRUE(decode(bytes, opts));
    EXPECT_STREQ(opts.name, "Pad");
    EXPECT_EQ(opts.deviceToEmulate, GuitarHeroGuitar);
}

TEST(ProfileOptsTest, MissingRequiredFieldsStillFail)
{
    const std::vector<uint8_t> bytes = {0x08, 0x2A};
    proto_ProfileOpts opts;
    EXPECT_FALSE(decode(bytes, opts));
}

TEST(ProfileOptsTest, TruncatedInputFails)
{
    auto bytes = opts_bytes(std::string(40, 'q'));
    for (size_t cut : {size_t(1), size_t(5), size_t(20), bytes.size() - 7})
    {
        SCOPED_TRACE(cut);
        proto_ProfileOpts opts;
        EXPECT_FALSE(decode(std::vector<uint8_t>(bytes.begin(), bytes.begin() + cut), opts));
    }
}
