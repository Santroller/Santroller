#pragma once

#include "gip_packet_handler.h"
#include "enums.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shared device type mappings for both wired and wireless Xbox controllers
// These map GIP device descriptor strings to SubType values
static const gip_device_type_mapping_t GIP_DEVICE_TYPE_MAPPINGS[] = {
    {Gamepad, "Windows.Xbox.Input.Gamepad"},
    {RockBandGuitar, "MadCatz.Xbox.Guitar.Stratocaster"},
    {RockBandGuitar, "PDP.Xbox.Guitar.Jaguar"},
    {LiveGuitar, "Activision.Xbox.Input.GH7"},
    {RockBandDrums, "MadCatz.Xbox.Drums.Glam"},
    {RockBandDrums, "PDP.Xbox.Drums.Tablah"},
    // {WirelessLegacyAdapter, "MadCatz.Xbox.Module.Brangus"},
    // {WiredLegacyAdapter, "PDP.Xbox.RBAdapter.LegacyUSB"},
    {Skylanders, "Activision.Xbox.Skylanders.Portal"},
    {LegoDimensions, "TTGames.Xbox.Dimensions.Gateway"},
    {DisneyInfinity, "Disney.Xbox.Infinity.Base"}
};

#define GIP_DEVICE_TYPE_MAPPING_COUNT (sizeof(GIP_DEVICE_TYPE_MAPPINGS) / sizeof(GIP_DEVICE_TYPE_MAPPINGS[0]))

#ifdef __cplusplus
}
#endif
