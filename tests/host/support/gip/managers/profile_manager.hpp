#pragma once
// Fake managers/profile_manager.hpp: the emulated devices only register their endpoints
#include <stdint.h>
#include "profiles/profile.hpp"
#include "instance.hpp"
#include "emulation/usb/device.hpp"

class ProfileManager
{
public:
    static ProfileManager &instance()
    {
        static ProfileManager manager;
        return manager;
    }
    void map_usb_instance_epin(uint8_t epin, uint8_t interface_id) { (void)epin, (void)interface_id; }
    void map_usb_instance_epout(uint8_t epout, uint8_t interface_id) { (void)epout, (void)interface_id; }
};
