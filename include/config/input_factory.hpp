#pragma once
#include <memory>
#include <stdint.h>
#include "input.pb.h"
#include "profiles/profile.hpp"

class Input;

class InputFactory {
public:
    // Create an input from protobuf
    static std::unique_ptr<Input> create_input(
        const proto_Input& proto_input,
        std::shared_ptr<Profile> profile
    );
    
    static bool has_device(std::shared_ptr<Profile> profile, uint32_t device_id);
    
    template<typename T>
    static std::shared_ptr<T> get_device(std::shared_ptr<Profile> profile, uint32_t device_id) {
        auto c_it = profile->claimed_devices.find(device_id);
        if (c_it != profile->claimed_devices.end()) {
            return std::static_pointer_cast<T>(c_it->second);
        }
        if (!profile->claimed_devices.empty()) {
            return std::static_pointer_cast<T>(profile->claimed_devices.begin()->second);
        }
        auto s_it = profile->devices.find(device_id);
        if (s_it != profile->devices.end()) {
            return std::static_pointer_cast<T>(s_it->second);
        }
        return nullptr;
    }
};
