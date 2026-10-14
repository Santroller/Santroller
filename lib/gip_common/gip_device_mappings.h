#pragma once

#include "gip_packet_handler.h"
#include "enums.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shared device type mappings for both wired and wireless Xbox controllers
// These map GIP device descriptor strings to SubType values, plus a display name for
// wireless controllers, which have no USB product string of their own
static const gip_device_type_mapping_t GIP_DEVICE_TYPE_MAPPINGS[] = {
    {RockBandGuitar, "MadCatz.Xbox.Guitar.Stratocaster", "Mad Catz Stratocaster"},
    {RockBandGuitar, "PDP.Xbox.Guitar.Jaguar", "PDP Jaguar Guitar"},
    {LiveGuitar, "Activision.Xbox.Input.GH7", "Guitar Hero Live Guitar"},
    {RockBandDrums, "MadCatz.Xbox.Drums.Glam", "Mad Catz Rock Band Drums"},
    {RockBandDrums, "PDP.Xbox.Drums.Tablah", "PDP Rock Band Drums"},
    // {WirelessLegacyAdapter, "MadCatz.Xbox.Module.Brangus"},
    // {WiredLegacyAdapter, "PDP.Xbox.RBAdapter.LegacyUSB"},
    {Skylanders, "Activision.Xbox.Skylanders.Portal", "Skylanders Portal"},
    {LegoDimensions, "TTGames.Xbox.Dimensions.Gateway", "Lego Dimensions Toy Pad"},
    {DisneyInfinity, "Disney.Xbox.Infinity.Base", "Disney Infinity Base"},
    {Gamepad, "Windows.Xbox.Input.Gamepad", "Xbox One Controller"}
};

#define GIP_DEVICE_TYPE_MAPPING_COUNT (sizeof(GIP_DEVICE_TYPE_MAPPINGS) / sizeof(GIP_DEVICE_TYPE_MAPPINGS[0]))

#ifdef __cplusplus
}
#endif
