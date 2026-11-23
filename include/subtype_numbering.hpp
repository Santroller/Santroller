#pragma once
#include <stdint.h>
#include "enums.pb.h"

// SubType values are valid BCD as they are, so the subtype can go straight into a revision (the BLE PnP ID version is
// subtype << 8), and match Santroller 1's numbering for the subtypes it had. Before that, a few subtypes had values
// with a hex digit above 9 (10-15 and 26-31). Those moved to free values, and nothing took their old ones, so an old
// value always means the same thing: configs, pairings and other Santrollers still on the old numbering are read
// through subtype_from_stored, which leaves every current value alone.

struct SubTypeRenumbering
{
    uint8_t old_value;
    SubType subtype;
};
static constexpr SubTypeRenumbering SUBTYPE_RENUMBERING[] = {
    {10, DisneyInfinity},
    {11, Skylanders},
    {12, LegoDimensions},
    {13, ProjectDiva},
    {14, GuitarFreaks},
    {15, Wheel},
    {26, PowerGigGuitar},
    {27, RockRevolutionGuitar},
    {28, Midi},
    {29, BeatMania},
    {30, DrumMania},
    {31, KeyboardMania},
};

// A subtype from a config, a saved pairing or another Santroller, which may use the old numbering
static constexpr SubType subtype_from_stored(uint32_t value)
{
    for (const auto &entry : SUBTYPE_RENUMBERING)
    {
        if (entry.old_value == value)
        {
            return entry.subtype;
        }
    }
    return (SubType)value;
}

// The value a subtype had in the old numbering. Where things were derived from the value and have to stay the
// same (the order slots are numbered in, each subtype's bluetooth address), they use this.
static constexpr uint8_t subtype_old_value(SubType subtype)
{
    for (const auto &entry : SUBTYPE_RENUMBERING)
    {
        if (entry.subtype == subtype)
        {
            return entry.old_value;
        }
    }
    return (uint8_t)subtype;
}

// Every subtype but Unknown is valid BCD (enums.proto); tests/host/config/subtype_numbering_test.cpp checks the enum
static constexpr bool subtype_is_bcd(uint32_t value)
{
    return value <= 0x99 && (value & 0x0F) <= 9;
}
static_assert(subtype_from_stored(15) == Wheel && subtype_from_stored(Wheel) == Wheel, "subtype renumbering");
static_assert(subtype_old_value(Wheel) == 15 && subtype_old_value(ProKeys) == 16, "subtype renumbering");
