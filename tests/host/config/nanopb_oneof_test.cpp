// GHSA-p24j-vqcp-x988: in a oneof mixing static and callback members, decoding a static member and then
// a callback member used to leave the static member's bytes in the union, which nanopb then called
// through as the callback's decode pointer. proto_Input's input oneof has both kinds.
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <pb_decode.h>
#include <pb_encode.h>
#include "input.pb.h"

namespace
{
bool s_hijacked = false;

bool hijacked(pb_istream_t *, const pb_field_t *, void **)
{
    s_hijacked = true;
    return true;
}

std::vector<uint8_t> encode(const pb_msgdesc_t *fields, const void *msg)
{
    std::vector<uint8_t> buffer(64);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());
    EXPECT_TRUE(pb_encode(&stream, fields, msg));
    buffer.resize(stream.bytes_written);
    return buffer;
}
}

TEST(NanopbOneofTest, ACallbackMemberAfterAStaticMemberDoesNotCallItsBytes)
{
    // The static member's fields line up with the callback's decode pointer, so point them at hijacked()
    proto_WiiAxisInput wii = proto_WiiAxisInput_init_zero;
    static_assert(sizeof(wii) >= sizeof(pb_callback_t::funcs), "wiiAxis must cover the decode pointer");
    auto target = &hijacked;
    memcpy(&wii, &target, sizeof(target));
    auto wii_bytes = encode(proto_WiiAxisInput_fields, &wii);

    std::vector<uint8_t> message = {(proto_Input_wiiAxis_tag << 3) | PB_WT_STRING, uint8_t(wii_bytes.size())};
    message.insert(message.end(), wii_bytes.begin(), wii_bytes.end());
    // The callback member with a varint wire type skips the submsg callback that would set its pointer
    message.push_back((proto_Input_shortcut_tag << 3) | PB_WT_VARINT);
    message.push_back(0);

    s_hijacked = false;
    proto_Input input = proto_Input_init_zero;
    pb_istream_t stream = pb_istream_from_buffer(message.data(), message.size());
    pb_decode(&stream, proto_Input_fields, &input);
    EXPECT_FALSE(s_hijacked);
    EXPECT_EQ(input.which_input, proto_Input_shortcut_tag);
}
