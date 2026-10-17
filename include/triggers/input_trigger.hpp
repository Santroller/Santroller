#pragma once
#include "triggers/base_trigger.hpp"
#include "input.pb.h"
#include "managers/config_manager.hpp"
#include <memory>

class Input;

class InputActivationTrigger : public ActivationTrigger
{
public:
    InputActivationTrigger(bool any_time, proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id);
    ~InputActivationTrigger() {}
    bool validate(bool claim_device, bool full_poll, bool send_events);
    void reset();
    int assignedDevices() { return 0; }
    bool reads_input() const override { return true; }
    bool awaiting_startup_input() const override;

protected:
    proto_InputActivationTrigger m_activation_trigger;
    std::unique_ptr<Input> m_input;
    bool m_any_time;
private:
    bool calc_val();
    StartupTriggerKey startup_key() const;
};
