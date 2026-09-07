#pragma once
#include <stdint.h>
#include <memory>
#include "config.pb.h"

class Profile;

class ActivationTrigger
{
public:
    ActivationTrigger(Profile *profile, uint32_t id, uint32_t list_id) : m_profile(profile), m_id(id), m_list_id(list_id) {}
    ActivationTrigger(const std::shared_ptr<Profile> &profile, uint32_t id, uint32_t list_id) : ActivationTrigger(profile.get(), id, list_id) {}
    virtual ~ActivationTrigger() {}
    virtual bool validate(bool claim_device, bool full_poll, bool send_events) = 0;
    virtual int assignedDevices() = 0;
    virtual bool forcedConsoleMode(ConsoleMode& mode) const { return false; }

protected:
    Profile *m_profile;
    bool m_initialised = false;
    bool m_last_val = false;
    uint16_t m_last_analog_val = 0;
    uint16_t m_id;
    uint16_t m_list_id;
};
