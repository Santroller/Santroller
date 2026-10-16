#pragma once
#include <memory>
#include "instance.hpp"
#include "profiles/profile.hpp"
#include "config.pb.h"

class UsbDevice;

class InstanceFactory {
public:
    static std::shared_ptr<Instance> create_instance(
        int assignment_mask,
        std::shared_ptr<Profile> profile,
        ConsoleMode usb_mode
    );
    
    static std::shared_ptr<UsbDevice> create_usb_instance(
        ConsoleMode mode,
        SubType subtype
    );

    // A bluetooth peripheral serving only the config service, for when no profile uses bluetooth
    static std::shared_ptr<Instance> create_bt_config_instance();

    // A PC takes a mix of interface types, so each profile on windows picks xinput or HID for itself
    static bool uses_xinput_on_windows(SubType subtype, bool xinput_on_windows);
    
private:
    static void setup_instance_from_profile(
        std::shared_ptr<Instance> instance,
        std::shared_ptr<Profile> profile
    );
};
