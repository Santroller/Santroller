#pragma once
#include <stdint.h>
#include <memory>

class Profile;

class ActivationTrigger
{
public:
    ActivationTrigger(std::shared_ptr<Profile> profile, uint32_t id, uint32_t list_id) : m_profile(profile), m_id(id), m_list_id(list_id) {}
    ~ActivationTrigger() {}
    virtual bool validate(bool claim_device, bool full_poll, bool send_events) = 0;
    virtual int assignedDevices() = 0;

protected:
    std::shared_ptr<Profile> m_profile;
    bool m_initialised = false;
    bool m_last_val = false;
    uint16_t m_last_analog_val = 0;
    uint16_t m_id;
    uint16_t m_list_id;
};
