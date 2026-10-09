#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "input.pb.h"

// proto_Output for a gamepad button / axis mapping, as the host tick helpers receive them
inline proto_Output gamepad_button(GamepadButtonType button)
{
    proto_Output out;
    memset(&out, 0, sizeof(out));
    out.which_mapping = proto_Output_gamepadButton_tag;
    out.mapping.gamepadButton = button;
    return out;
}

inline proto_Output gamepad_axis(GamepadAxisType axis)
{
    proto_Output out;
    memset(&out, 0, sizeof(out));
    out.which_mapping = proto_Output_gamepadAxis_tag;
    out.mapping.gamepadAxis = axis;
    return out;
}

// Any mapping that isn't a gamepad one, which the gamepad tick helpers must ignore
inline proto_Output non_gamepad_mapping(uint32_t value)
{
    proto_Output out;
    memset(&out, 0, sizeof(out));
    out.which_mapping = proto_Output_ghButton_tag;
    // same storage as gamepadButton / gamepadAxis, so a helper that ignores which_mapping would see it
    memcpy(&out.mapping, &value, sizeof(value));
    return out;
}

// Raw bytes of a report struct, for checking bitfield packing against the wire format
template <typename T>
const uint8_t *raw_bytes(const T &report)
{
    static_assert(std::is_trivially_copyable_v<T>);
    return reinterpret_cast<const uint8_t *>(&report);
}

// Sets one bitfield of a zeroed report and checks exactly that bit of the wire format is set
#define EXPECT_BIT_AT(Type, field, byte, mask)                                       \
    do                                                                               \
    {                                                                                \
        Type report_;                                                                \
        memset(&report_, 0, sizeof(report_));                                        \
        report_.field = 1;                                                           \
        const uint8_t *bytes_ = raw_bytes(report_);                                  \
        for (size_t i_ = 0; i_ < sizeof(Type); i_++)                                 \
            EXPECT_EQ(bytes_[i_], i_ == (byte) ? (mask) : 0)                         \
                << #Type "." #field " should be byte " << (byte) << " mask " << (mask) \
                << ", differs at byte " << i_;                                       \
    } while (0)

// Same for an 8 bit field, which may be a bitfield (so offsetof can't be used)
#define EXPECT_BYTE_AT(Type, field, byte) EXPECT_BIT_AT_VALUE(Type, field, byte, 0xFF)
#define EXPECT_BIT_AT_VALUE(Type, field, byte, value)                                \
    do                                                                               \
    {                                                                                \
        Type report_;                                                                \
        memset(&report_, 0, sizeof(report_));                                        \
        report_.field = (value);                                                     \
        const uint8_t *bytes_ = raw_bytes(report_);                                  \
        for (size_t i_ = 0; i_ < sizeof(Type); i_++)                                 \
            EXPECT_EQ(bytes_[i_], i_ == (byte) ? (value) : 0)                        \
                << #Type "." #field " should be byte " << (byte) << ", differs at byte " << i_; \
    } while (0)
