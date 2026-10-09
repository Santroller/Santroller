// lib/gip_common/gip_packet_handler.cpp: what the USB host reads from a controller's GIP metadata
// (message 0x04) - the device type from its preferred type strings, and where the
// IConsoleFunctionMap (Share button) extension sits in its input report. Inputs are the metadata
// example in [MS-GIPUSB] 2.2.2 and real devices' metadata dumped in PlasticBand.
#include <gtest/gtest.h>

#include "gip_device_mappings.h"
#include "gip_packet_handler.h"
#include "gip_test_support.hpp"
#include "plasticband_metadata.hpp"

using namespace gip_test;
namespace pb = gip_test::plasticband;

namespace
{
uint8_t detect(const Bytes &metadata)
{
    return gip_detect_device_subtype(metadata.data(), metadata.size(), GIP_DEVICE_TYPE_MAPPINGS, GIP_DEVICE_TYPE_MAPPING_COUNT);
}

uint16_t console_function_offset(const Bytes &metadata)
{
    return gip_parse_console_function_offset(metadata.data(), metadata.size());
}
} // namespace

// The test support metadata builder lays blobs out the way the spec's compiled example does
TEST(GipMetadataBuilder, ReproducesTheSpecExample)
{
    Bytes built = metadata({"Windows.Xbox.Input.Gamepad"}, {GUID_ICONTROLLER, GUID_IGAMEPAD, GUID_INAVIGATION},
                           {{0x20, 14, true}, {0x09, 9, false}});
    EXPECT_EQ(built, SPEC_GAMEPAD_METADATA);
}

TEST(GipMetadata, SpecGamepadIsAGamepad)
{
    EXPECT_EQ(detect(SPEC_GAMEPAD_METADATA), Gamepad);
}

// Class strings and the devices they belong to, from PlasticBand's Xbox One instrument docs
// (Instruments/5-Fret Guitar/Rock Band/Xbox One.md, 4-Lane Drums/Xbox One.md,
// 6-Fret Guitar/Xbox One.md), checked on the metadata those devices actually send
TEST(GipMetadata, RealDevicesAreDetected)
{
    EXPECT_EQ(detect(pb::MICROSOFT_GAMEPAD_METADATA), Gamepad);
    EXPECT_EQ(detect(pb::MADCATZ_STRATOCASTER_METADATA), RockBandGuitar);
    EXPECT_EQ(detect(pb::PDP_JAGUAR_METADATA), RockBandGuitar);
    EXPECT_EQ(detect(pb::PDP_RIFFMASTER_METADATA), RockBandGuitar);
    EXPECT_EQ(detect(pb::MADCATZ_DRUMS_METADATA), RockBandDrums);
    EXPECT_EQ(detect(pb::PDP_DRUMS_METADATA), RockBandDrums);
    // GH7 is listed before Windows.Xbox.Input.Gamepad; the first (preferred) type wins
    EXPECT_EQ(detect(pb::GHL_DONGLE_METADATA), LiveGuitar);
}

TEST(GipMetadata, PreferredTypesAreCheckedInOrder)
{
    EXPECT_EQ(detect(metadata({"Windows.Xbox.Input.NavigationController", "PDP.Xbox.Drums.Tablah"}, {GUID_INAVIGATION}, {{0x20, 10, true}})),
              RockBandDrums);
    EXPECT_EQ(detect(metadata({"Activision.Xbox.Input.GH7", "Windows.Xbox.Input.Gamepad"}, {}, {{0x20, 14, true}})), LiveGuitar);
}

TEST(GipMetadata, UnknownTypes)
{
    EXPECT_EQ(detect(metadata({"Windows.Xbox.Input.NavigationController"}, {GUID_INAVIGATION}, {{0x20, 14, true}})), 0xFF);
    EXPECT_EQ(detect(metadata({}, {}, {})), 0xFF);
    EXPECT_EQ(detect(Bytes(SPEC_GAMEPAD_METADATA.begin(), SPEC_GAMEPAD_METADATA.begin() + 20)), 0xFF);
}

// A preferred type string that claims to run past the end of the metadata stops the search
TEST(GipMetadata, OverlongStringIsNotRead)
{
    Bytes md = metadata({"PDP.Xbox.Drums.Tablah"}, {}, {});
    // preferred types table: 16 byte header + offset at device metadata +10
    uint16_t preferred = md[16 + 10] | md[16 + 11] << 8;
    size_t len_at = 16 + preferred + 1;
    md[len_at] = 0xFF;
    md[len_at + 1] = 0x7F;
    EXPECT_EQ(detect(md), 0xFF);
}

// [MS-GIPUSB] 3.1.5.6.1.3: extensions are appended after the input report payload and the
// metadata length for message 0x20 includes them; the console function map is 18 bytes
// (3.1.5.6.1.3.1). No IConsoleFunctionMap interface means no Share button.
TEST(GipConsoleFunctionMap, AbsentWithoutTheInterface)
{
    EXPECT_EQ(console_function_offset(SPEC_GAMEPAD_METADATA), 0);
    EXPECT_EQ(console_function_offset(pb::MICROSOFT_GAMEPAD_METADATA), 0);
    EXPECT_EQ(console_function_offset(pb::MADCATZ_STRATOCASTER_METADATA), 0);
    EXPECT_EQ(console_function_offset(pb::MADCATZ_DRUMS_METADATA), 0);
}

// [MS-GIPUSB] Table 26: "Gamepad Input Report w/ Share Button" is 0x20 bytes, i.e. the 14 byte
// report then the 18 byte map at offset 14. xpad reads the Share bit from data[len - 18].
TEST(GipConsoleFunctionMap, GamepadWithShare)
{
    Bytes md = metadata({"Windows.Xbox.Input.Gamepad"}, {GUID_ICONTROLLER, GUID_IGAMEPAD, GUID_ICONSOLE_FUNCTION_MAP, GUID_INAVIGATION},
                        {{0x20, 0x20, true}, {0x09, 9, false}});
    EXPECT_EQ(console_function_offset(md), 14);
}

// The Riffmaster's metadata (PlasticBand dump) lists IConsoleFunctionMap with a 32 byte 0x20
// message; PlasticBand's Riffmaster report puts the console function map at byte 14
TEST(GipConsoleFunctionMap, Riffmaster)
{
    EXPECT_EQ(console_function_offset(pb::PDP_RIFFMASTER_METADATA), 14);
}

TEST(GipConsoleFunctionMap, MessageShorterThanTheMap)
{
    Bytes md = metadata({"Windows.Xbox.Input.Gamepad"}, {GUID_ICONSOLE_FUNCTION_MAP}, {{0x20, 10, true}});
    EXPECT_EQ(console_function_offset(md), 0);
}

TEST(GipConsoleFunctionMap, TruncatedMetadata)
{
    Bytes md = pb::PDP_RIFFMASTER_METADATA;
    md.resize(120);
    EXPECT_EQ(console_function_offset(md), 0);
    EXPECT_EQ(console_function_offset(Bytes(md.begin(), md.begin() + 30)), 0);
}
