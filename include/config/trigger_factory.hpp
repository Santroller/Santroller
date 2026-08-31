#pragma once
#include <memory>
#include <stdint.h>
#include "input.pb.h"
#include "config.pb.h"

class ActivationTrigger;
class Profile;
class Input;

class TriggerFactory {
public:
    // Create an activation trigger from protobuf
    static std::unique_ptr<ActivationTrigger> create_trigger(
        const proto_ProfileAssignmentInfo& proto_assignment,
        std::shared_ptr<Profile> profile,
        std::unique_ptr<Input> input,
        uint32_t trigger_id,
        uint32_t list_id
    );
};
