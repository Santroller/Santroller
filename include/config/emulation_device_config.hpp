#pragma once

#include "device.pb.h"

#include <cstring>

struct EmulationDeviceConfig {
    EmulationDeviceConfig()
        : has_psx(false),
          has_wii(false)
    {
        memset(&psx, 0, sizeof(psx));
        memset(&wii, 0, sizeof(wii));
    }

    bool has_psx;
    proto_PSXEmulationDevice psx;
    bool has_wii;
    proto_WiiEmulationDevice wii;
};