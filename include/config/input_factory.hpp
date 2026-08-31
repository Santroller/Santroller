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
        if (!has_device(profile, device_id)) {
            return nullptr;
        }
        return std::static_pointer_cast<T>(profile->devices[device_id]);
    }
};
