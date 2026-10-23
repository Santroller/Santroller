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
    // Puts input triggers after the rest, once the list has been loaded
    void order_triggers();
    int assignedDevices();
    bool forcedConsoleMode(ConsoleMode& mode) const;
    bool xinputOnWindows(bool &enabled) const;
    bool ps4OrPs5Mode(bool &enabled) const;
    // Matched without its controllers, as Xbox One auth held it up
    bool held() const { return m_held; }
    bool claimed() const { return m_claimed; }
private:
    bool validate_triggers(bool claim_devices, bool full_poll, bool send_events, bool held, bool &claimable);
    bool m_claimed = false;
    bool m_held = false;
    bool m_last_val = false;
};
