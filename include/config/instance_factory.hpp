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
        std::shared_ptr<Profile> profile
    );
    
    static std::shared_ptr<UsbDevice> create_usb_instance(
        ConsoleMode mode,
        SubType subtype
    );
    
private:
    static void setup_instance_from_profile(
        std::shared_ptr<Instance> instance,
        std::shared_ptr<Profile> profile
    );
};
