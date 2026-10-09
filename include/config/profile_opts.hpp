#pragma once

#include <stdint.h>
#include <string.h>
#include <pb_common.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include "config.pb.h"

// Decode a profile's ProfileOpts. The firmware only holds sizeof(opts->name) - 1 bytes of the name, and
// nanopb fails the whole message on a longer one, which would leave the profile without its uid or type.
// Older config tools don't limit the name, so a long one is cut short (on a UTF-8 character boundary)
// instead: the fields are copied into a buffer with the name shortened, and that is decoded as usual.
inline bool decode_profile_opts(pb_istream_t *stream, proto_ProfileOpts *opts)
{
    pb_byte_t buffer[proto_ProfileOpts_size];
    pb_ostream_t out = pb_ostream_from_buffer(buffer, sizeof(buffer));
    while (stream->bytes_left)
    {
        pb_wire_type_t wire_type;
        uint32_t tag;
        bool eof;
        if (!pb_decode_tag(stream, &wire_type, &tag, &eof))
        {
            if (eof)
            {
                break;
            }
            return false;
        }
        pb_field_iter_t field;
        if (!pb_field_iter_begin(&field, proto_ProfileOpts_fields, opts) || !pb_field_iter_find(&field, tag))
        {
            // nanopb ignores fields it doesn't know about, so leave them out
            if (!pb_skip_field(stream, wire_type))
            {
                return false;
            }
            continue;
        }
        if (!pb_encode_tag(&out, wire_type, tag))
        {
            return false;
        }
        switch (wire_type)
        {
        case PB_WT_VARINT:
        {
            uint64_t value;
            if (!pb_decode_varint(stream, &value) || !pb_encode_varint(&out, value))
            {
                return false;
            }
            break;
        }
        case PB_WT_32BIT:
        case PB_WT_64BIT:
        {
            pb_byte_t value[8];
            const size_t size = wire_type == PB_WT_32BIT ? 4 : 8;
            if (!pb_read(stream, value, size) || !pb_write(&out, value, size))
            {
                return false;
            }
            break;
        }
        case PB_WT_STRING:
        {
            pb_istream_t substream;
            if (!pb_make_string_substream(stream, &substream))
            {
                return false;
            }
            size_t length = substream.bytes_left;
            if (tag == proto_ProfileOpts_name_tag && length > sizeof(opts->name) - 1)
            {
                length = sizeof(opts->name) - 1;
            }
            pb_byte_t value[sizeof(opts->name)];
            if (length > sizeof(value) || !pb_read(&substream, value, length))
            {
                return false;
            }
            // Don't leave half a UTF-8 character at the end of a shortened name
            if (substream.bytes_left > 0)
            {
                pb_byte_t next;
                pb_istream_t peek = substream;
                if (pb_read(&peek, &next, 1) && (next & 0xC0) == 0x80)
                {
                    while (length > 0 && (value[length - 1] & 0xC0) == 0x80)
                    {
                        length--;
                    }
                    if (length > 0 && (value[length - 1] & 0xC0) == 0xC0)
                    {
                        length--;
                    }
                }
            }
            if (!pb_close_string_substream(stream, &substream) ||
                !pb_encode_varint(&out, length) || !pb_write(&out, value, length))
            {
                return false;
            }
            break;
        }
        default:
            return false;
        }
    }
    pb_istream_t shortened = pb_istream_from_buffer(buffer, out.bytes_written);
    return pb_decode_ex(&shortened, proto_ProfileOpts_fields, opts, PB_DECODE_NOINIT);
}
