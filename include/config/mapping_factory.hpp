#pragma once
#include <memory>
#include <stdint.h>
#include "input.pb.h"
#include "config.pb.h"

class Mapping;
class Profile;
class Input;

class MappingFactory {
public:
    // Create a mapping from protobuf
    static std::unique_ptr<Mapping> create_mapping(
        const proto_Mapping& proto_mapping,
        std::shared_ptr<Profile> profile,
        std::unique_ptr<Input> input,
        uint32_t mapping_id
    );
};
