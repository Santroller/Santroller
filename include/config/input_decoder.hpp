#pragma once

#include <memory>
#include <pb_decode.h>
#include "input.pb.h"
#include "input/input.hpp"

class Profile;

// Each Input field needs its own result, including siblings inside the same modifier.
struct InputDecoder
{
    std::shared_ptr<Profile> profile;
    std::unique_ptr<Input> special;

    std::unique_ptr<Input> take(const proto_Input &input);
    static bool prepare(pb_istream_t *stream, const pb_field_t *field, void **arg);
    static bool decode_special(pb_istream_t *stream, const pb_field_t *field, void **arg);
};
