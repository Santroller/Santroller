#pragma once
#include <memory>
#include <stdint.h>
#include <vector>
#include "input.pb.h"
#include "config.pb.h"
#include "leds/display_element_data.hpp"

class LedMappingDevice;
class LedMapping;
class Profile;
class Input;

class LedFactory
{
public:
    // Create LED device from protobuf. display is a display LED's image and animation, which are decoded
    // separately
    static std::unique_ptr<LedMappingDevice> create_led_device(
        const proto_LedDevice &proto_led_device,
        std::shared_ptr<Profile> profile,
        DisplayElementData display = {});

    // Create LED mapping from protobuf
    static std::unique_ptr<LedMapping> create_led_mapping(
        const proto_LedMapping &proto_led_mapping,
        std::shared_ptr<Profile> profile,
        std::unique_ptr<LedMappingDevice> device,
        std::unique_ptr<Input> input,
        uint32_t led_id);
};
