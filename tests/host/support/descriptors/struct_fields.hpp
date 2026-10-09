#pragma once
// Where a report struct field sits on the wire, found by setting it in a zeroed struct (so bitfields
// work too), and gtest checks that a descriptor element describes that field.
//
// Bit numbering is the HID one: bit n of a report is bit (n % 8) of byte (n / 8), so a little endian
// multi-byte field is a contiguous run of bits. The packed report structs lay out the same on the host
// (x86-64 / AArch64, GCC / Clang) as on the RP2040 (little endian, bitfields allocated from the least
// significant bit, per the AAPCS), so positions found here are the firmware's.
#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>
#include "hid_descriptor.hpp"

namespace hid_desc
{

struct FieldBits
{
    uint32_t bit = 0;   // first bit, from the start of the struct
    uint32_t width = 0; // number of bits
    bool is_signed = false;
    bool contiguous = true;
    const char *name = "";
};

template <typename T, typename Setter>
FieldBits field_bits(Setter set, bool is_signed, const char *name)
{
    static_assert(std::is_trivially_copyable_v<T>);
    T report;
    memset(&report, 0, sizeof(report));
    set(report, ~uint64_t(0));
    uint8_t bytes[sizeof(T)];
    memcpy(bytes, &report, sizeof(T));
    FieldBits out;
    out.is_signed = is_signed;
    out.name = name;
    bool found = false;
    uint32_t last = 0;
    for (uint32_t i = 0; i < sizeof(T) * 8; i++)
    {
        if (!(bytes[i / 8] & (1u << (i % 8))))
            continue;
        if (!found)
        {
            out.bit = i;
            found = true;
        }
        else if (i != last + 1)
        {
            out.contiguous = false;
        }
        last = i;
        out.width++;
    }
    return out;
}

// FIELD(Type, member) works for plain members and bitfields alike
#define FIELD(Type, member)                                                                  \
    ::hid_desc::field_bits<Type>([](Type &r_, uint64_t v_) { r_.member = v_; },             \
                                 std::is_signed_v<decltype(Type::member)>, #Type "." #member)

// An array member (e.g. vendor bytes): its byte range as one field
#define ARRAY_FIELD(Type, member)                                                            \
    ::hid_desc::FieldBits{uint32_t(offsetof(Type, member) * 8), uint32_t(sizeof(Type::member) * 8), false, true, #Type "." #member}

// Checks the descriptor has exactly one data element with this usage in the report, at the field's bit
// position and with its width, and a logical range the field can hold (signed fields need a negative
// minimum, unsigned ones a non-negative one). struct_has_report_id: the struct starts with the report id
// byte, which descriptors don't count.
inline void expect_field(const Descriptor &d, ReportType type, uint8_t report_id, uint32_t usage, const FieldBits &field,
                         bool struct_has_report_id)
{
    std::ostringstream where;
    where << field.name << " vs " << to_string(type) << " report " << int(report_id) << " usage 0x" << std::hex << usage;
    SCOPED_TRACE(where.str());
    ASSERT_TRUE(field.contiguous) << "field bits aren't contiguous";
    ASSERT_GT(field.width, 0u) << "field not found in the struct";
    auto all = d.find_all(type, report_id, usage);
    ASSERT_EQ(all.size(), 1u) << "expected exactly one element with this usage";
    const Element &e = *all[0];
    const uint32_t struct_bit = e.bit_offset + (struct_has_report_id ? 8 : 0);
    EXPECT_EQ(struct_bit, field.bit) << "descriptor puts the usage at byte " << struct_bit / 8 << " bit " << struct_bit % 8
                                     << ", the struct field is at byte " << field.bit / 8 << " bit " << field.bit % 8;
    EXPECT_EQ(e.bit_size, field.width) << "descriptor report size vs struct field width";
    EXPECT_TRUE(e.variable()) << "a field must be a variable item";
    if (field.is_signed)
    {
        EXPECT_LT(e.logical_min, 0) << "signed field needs a negative logical minimum";
        EXPECT_GE(e.logical_min, -(int64_t(1) << (field.width - 1)));
        EXPECT_LE(e.logical_max, (int64_t(1) << (field.width - 1)) - 1);
    }
    else
    {
        EXPECT_GE(e.logical_min, 0) << "unsigned field needs a non-negative logical minimum";
        EXPECT_LE(e.logical_max, (int64_t(1) << field.width) - 1) << "logical maximum doesn't fit the field";
    }
}

// The same for a run of bytes the descriptor describes as one usage per byte (vendor blocks)
inline void expect_bytes(const Descriptor &d, ReportType type, uint8_t report_id, uint32_t first_bit_in_report,
                         const FieldBits &field, bool struct_has_report_id)
{
    SCOPED_TRACE(field.name);
    const uint32_t struct_bit = first_bit_in_report + (struct_has_report_id ? 8 : 0);
    EXPECT_EQ(struct_bit, field.bit);
    for (uint32_t bit = first_bit_in_report; bit < first_bit_in_report + field.width; bit += 8)
    {
        const Element *e = d.element_at(type, report_id, bit);
        ASSERT_NE(e, nullptr) << "no element at report bit " << bit;
        EXPECT_FALSE(e->constant()) << "bit " << bit << " is padding";
    }
}

} // namespace hid_desc
