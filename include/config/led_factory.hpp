#pragma once
#include <memory>
#include <stdint.h>
#include "input.pb.h"
#include "config.pb.h"

class LedMappingDevice;
class LedMapping;
class Profile;
class Input;

class LedFactory {
public:
    // Create LED device from protobuf
    static std::unique_ptr<LedMappingDevice> create_led_device(
        const proto_LedDevice& proto_led_device,
        std::shared_ptr<Profile> profile
    );
    
    // Create LED mapping from protobuf
    static std::unique_ptr<LedMapping> create_led_mapping(
        const proto_LedMapping& proto_led_mapping,
        std::shared_ptr<Profile> profile,
        std::unique_ptr<LedMappingDevice> device,
        std::unique_ptr<Input> input,
        uint32_t led_id
    );
};
