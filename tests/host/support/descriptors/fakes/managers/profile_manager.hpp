#pragma once
// Fake profile manager for compiling spice2x_device.cpp: endpoint mapping is a no-op
#include <cstdint>
class ProfileManager
{
public:
    static ProfileManager &instance()
    {
        static ProfileManager manager;
        return manager;
    }
    void map_usb_instance_epin(uint8_t, uint8_t) {}
    void map_usb_instance_epout(uint8_t, uint8_t) {}
};
