#pragma once
#include <vector>
#include <memory>
#include <stdint.h>

class ActivationTrigger;

class ActivationTriggerList
{
public:
    ~ActivationTriggerList();
    uint32_t profile_id;
    std::vector<std::unique_ptr<ActivationTrigger>> triggers;
    bool validate(bool claim_devices, bool full_poll, bool send_events);
    int assignedDevices();
private:
    bool m_claimed = false;
};
