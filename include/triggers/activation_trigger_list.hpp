#pragma once
#include <vector>
#include <memory>
#include <stdint.h>
#include "config.pb.h"

class ActivationTrigger;

class ActivationTriggerList
{
public:
    ~ActivationTriggerList();
    uint32_t profile_id;
    uint32_t list_id;
    std::vector<std::unique_ptr<ActivationTrigger>> triggers;
    bool validate(bool claim_devices, bool full_poll, bool send_events);
    int assignedDevices();
    bool forcedConsoleMode(ConsoleMode& mode) const;
    bool claimed() const { return m_claimed; }
private:
    bool m_claimed = false;
    bool m_last_val = false;
};
