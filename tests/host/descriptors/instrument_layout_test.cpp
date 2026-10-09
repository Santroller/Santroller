#include <gtest/gtest.h>
#include "firmware_descriptors.hpp"
#include "protocols/hid.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/ps5.hpp"
#include "protocols/xinput.hpp"
#include "struct_fields.hpp"

// Each emulated device has one report descriptor, but the mappings write a subtype specific struct
// (instrument reports) over the report buffer before it is sent. Where those structs use the generic
// controls (dpad, face buttons, start / PS) they have to land on the bits the descriptor declares for
// them, and the dpad bits on the nibble the device turns into a hat (dpad_bindings).
//
// Instrument specific positions (frets on other buttons, Hero Power on R3, ...) are deliberate and not
// checked here; only fields that share a name with the device's base report struct are.

using namespace hid_desc;

#define EXPECT_SAME_BIT(Base, base_field, Other, other_field) \
    EXPECT_EQ(FIELD(Other, other_field).bit, FIELD(Base, base_field).bit) << #Other "." #other_field " vs " #Base "." #base_field

// dpadUp / Down / Left / Right are the bits 0 - 3 of the base report's hat nibble
#define EXPECT_DPAD_ON_HAT(Base, Other)                                                  \
    do                                                                                 \
    {                                                                                  \
        const uint32_t hat_ = FIELD(Base, dpad).bit;                                   \
        EXPECT_EQ(FIELD(Other, dpadUp).bit, hat_ + 0) << #Other ".dpadUp";             \
        EXPECT_EQ(FIELD(Other, dpadDown).bit, hat_ + 1) << #Other ".dpadDown";         \
        EXPECT_EQ(FIELD(Other, dpadLeft).bit, hat_ + 2) << #Other ".dpadLeft";         \
        EXPECT_EQ(FIELD(Other, dpadRight).bit, hat_ + 3) << #Other ".dpadRight";       \
    } while (0)

#define EXPECT_FACE_BUTTONS(Base, Other)      \
    do                                        \
    {                                         \
        EXPECT_SAME_BIT(Base, a, Other, a);   \
        EXPECT_SAME_BIT(Base, b, Other, b);   \
        EXPECT_SAME_BIT(Base, x, Other, x);   \
        EXPECT_SAME_BIT(Base, y, Other, y);   \
    } while (0)

// ---------------------------------------------------------------------------------------------
// The HID gamepad sends the XInput style report (hid_gamepad_device.cpp), its hat replacing the dpad bits

TEST(InstrumentLayout, HidXInputInstrumentsUseTheGamepadBits)
{
    EXPECT_DPAD_ON_HAT(PCGamepadDpad_Data_t, XInputGamepad_Data_t);
    EXPECT_DPAD_ON_HAT(PCGamepadDpad_Data_t, XInputRockBandDrums_Data_t);
    EXPECT_DPAD_ON_HAT(PCGamepadDpad_Data_t, XInputGuitarHeroDrums_Data_t);
    EXPECT_DPAD_ON_HAT(PCGamepadDpad_Data_t, XInputGuitarHeroGuitar_Data_t);
    EXPECT_DPAD_ON_HAT(PCGamepadDpad_Data_t, XInputRockBandGuitar_Data_t);
    EXPECT_DPAD_ON_HAT(PCGamepadDpad_Data_t, XInputGHLGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PCGamepadDpad_Data_t, XInputRockBandDrums_Data_t);
    EXPECT_FACE_BUTTONS(PCGamepadDpad_Data_t, XInputGuitarHeroDrums_Data_t);
    EXPECT_FACE_BUTTONS(PCGamepadDpad_Data_t, XInputGuitarHeroGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PCGamepadDpad_Data_t, XInputRockBandGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PCGamepadDpad_Data_t, XInputGHLGuitar_Data_t);
    EXPECT_SAME_BIT(PCGamepadDpad_Data_t, start, XInputGHLGuitar_Data_t, start);
    EXPECT_SAME_BIT(PCGamepadDpad_Data_t, guide, XInputRockBandDrums_Data_t, guide);
    EXPECT_SAME_BIT(PCGamepadDpad_Data_t, guide, XInputGuitarHeroGuitar_Data_t, guide);
}

// ---------------------------------------------------------------------------------------------
// PS3 third party: the instruments' dpad bits are the PS3Dpad_Data_t hat nibble (ps3_device.cpp
// converts it for every non DS3 subtype)

TEST(InstrumentLayout, Ps3InstrumentsUseTheHatNibble)
{
    EXPECT_DPAD_ON_HAT(PS3Dpad_Data_t, PS3ThirdPartyGamepad_Data_t);
    EXPECT_DPAD_ON_HAT(PS3Dpad_Data_t, PS3RockBandDrums_Data_t);
    EXPECT_DPAD_ON_HAT(PS3Dpad_Data_t, PS3GuitarHeroGuitar_Data_t);
    EXPECT_DPAD_ON_HAT(PS3Dpad_Data_t, PS3RockBandGuitar_Data_t);
    EXPECT_DPAD_ON_HAT(PS3Dpad_Data_t, PS3GHLGuitar_Data_t);
    EXPECT_DPAD_ON_HAT(PS3Dpad_Data_t, PS3DJHTurntable_Data_t);
    EXPECT_SAME_BIT(PS3Dpad_Data_t, start, PS3RockBandDrums_Data_t, start);
    EXPECT_SAME_BIT(PS3Dpad_Data_t, guide, PS3RockBandDrums_Data_t, guide);
    EXPECT_SAME_BIT(PS3Dpad_Data_t, start, PS3GuitarHeroGuitar_Data_t, start);
    EXPECT_SAME_BIT(PS3Dpad_Data_t, guide, PS3GuitarHeroGuitar_Data_t, guide);
    EXPECT_SAME_BIT(PS3Dpad_Data_t, start, PS3GHLGuitar_Data_t, start);
    EXPECT_SAME_BIT(PS3Dpad_Data_t, guide, PS3DJHTurntable_Data_t, guide);
    EXPECT_FACE_BUTTONS(PS3Dpad_Data_t, PS3ThirdPartyGamepad_Data_t);
}

// ---------------------------------------------------------------------------------------------
// PS4: instruments are written over PS4Dpad_Data_t and ps4_device.cpp converts its dpad nibble to a hat

TEST(InstrumentLayout, Ps4InstrumentsUseTheGamepadBits)
{
    EXPECT_DPAD_ON_HAT(PS4Dpad_Data_t, PS4RockBandGuitar_Data_t);
    EXPECT_DPAD_ON_HAT(PS4Dpad_Data_t, PS4RockBandDrums_Data_t);
    EXPECT_DPAD_ON_HAT(PS4Dpad_Data_t, PS4GHLGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PS4Dpad_Data_t, PS4RockBandGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PS4Dpad_Data_t, PS4RockBandDrums_Data_t);
    EXPECT_FACE_BUTTONS(PS4Dpad_Data_t, PS4GHLGuitar_Data_t);
    EXPECT_SAME_BIT(PS4Dpad_Data_t, start, PS4RockBandGuitar_Data_t, start);
    EXPECT_SAME_BIT(PS4Dpad_Data_t, guide, PS4RockBandGuitar_Data_t, guide);
    EXPECT_SAME_BIT(PS4Dpad_Data_t, guide, PS4RockBandDrums_Data_t, guide);
    EXPECT_SAME_BIT(PS4Dpad_Data_t, start, PS4GHLGuitar_Data_t, start);
    EXPECT_SAME_BIT(PS4Dpad_Data_t, guide, PS4GHLGuitar_Data_t, guide);
}

// ---------------------------------------------------------------------------------------------
// PS5: the same over PS5Dpad_Data_t (ps5_device.cpp converts byte 8's low nibble to the hat)

TEST(InstrumentLayout, Ps5GamepadAndGuitarUseTheGamepadBits)
{
    EXPECT_DPAD_ON_HAT(PS5Dpad_Data_t, PS5Gamepad_Data_t);
    EXPECT_DPAD_ON_HAT(PS5Dpad_Data_t, PS5RockBandGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PS5Dpad_Data_t, PS5Gamepad_Data_t);
    EXPECT_FACE_BUTTONS(PS5Dpad_Data_t, PS5RockBandGuitar_Data_t);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, start, PS5RockBandGuitar_Data_t, start);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, guide, PS5RockBandGuitar_Data_t, guide);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, guide, PS5Gamepad_Data_t, guide);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, leftTrigger, PS5RockBandGuitar_Data_t, leftTrigger);
}

// PS5RockBandDrums_Data_t and PS5GHLGuitar_Data_t need the byte at offset 7 that PS5Dpad_Data_t has (the
// descriptor's vendor 0x20 byte). Without it their dpad and face buttons land in byte 7 instead of the
// hat + buttons 1 - 4 in byte 8, kick1 / kick2 land in the hat nibble (which ps5_device.cpp converts as
// the dpad), and guide lands on the L1 / R1 byte.
TEST(InstrumentLayout, Ps5DrumsAndGhlUseTheGamepadBits)
{
    EXPECT_DPAD_ON_HAT(PS5Dpad_Data_t, PS5RockBandDrums_Data_t);
    EXPECT_DPAD_ON_HAT(PS5Dpad_Data_t, PS5GHLGuitar_Data_t);
    EXPECT_FACE_BUTTONS(PS5Dpad_Data_t, PS5RockBandDrums_Data_t);
    EXPECT_FACE_BUTTONS(PS5Dpad_Data_t, PS5GHLGuitar_Data_t);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, guide, PS5RockBandDrums_Data_t, guide);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, guide, PS5GHLGuitar_Data_t, guide);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, start, PS5RockBandDrums_Data_t, start);
    EXPECT_SAME_BIT(PS5Dpad_Data_t, start, PS5GHLGuitar_Data_t, start);
    // the same packet counter / vendor block start as the guitar
    EXPECT_EQ(offsetof(PS5RockBandDrums_Data_t, packetCounter), offsetof(PS5RockBandGuitar_Data_t, packetCounter));
    EXPECT_EQ(offsetof(PS5GHLGuitar_Data_t, packetCounter), offsetof(PS5RockBandGuitar_Data_t, packetCounter));
    EXPECT_EQ(sizeof(PS5RockBandDrums_Data_t), sizeof(PS5Dpad_Data_t));
    EXPECT_EQ(sizeof(PS5GHLGuitar_Data_t), sizeof(PS5Dpad_Data_t));
}
