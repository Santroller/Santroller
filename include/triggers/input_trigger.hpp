#pragma once
#include "triggers/base_trigger.hpp"
#include "input.pb.h"
#include <memory>

class Input;

class InputActivationTrigger : public ActivationTrigger
{
public:
    InputActivationTrigger(bool any_time, proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~InputActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    int assignedDevices() { return 0; }

protected:
    proto_InputActivationTrigger m_activation_trigger;
    std::unique_ptr<Input> m_input;
    bool m_any_time;
};
