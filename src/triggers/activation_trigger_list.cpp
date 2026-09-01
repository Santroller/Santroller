#include "triggers/activation_trigger_list.hpp"
#include "triggers/activation_trigger.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"

// Destructor must be defined where ActivationTrigger is a complete type
ActivationTriggerList::~ActivationTriggerList() = default;

bool ActivationTriggerList::validate(bool claim_devices, bool full_poll, bool send_events)
{
    if (claim_devices && m_claimed)
    {
        return true;
    }
    if (triggers.empty())
    {
        return false;
    }
    for (auto &trigger : triggers)
    {
        if (!trigger->validate(false, full_poll, send_events))
        {
            if (claim_devices)
            {
                return false;
            }
        }
    }
    if (!claim_devices)
    {
        return true;
    }
    for (auto &trigger : triggers)
    {
        if (!trigger->validate(true, full_poll, send_events))
        {
            printf("trigger was not valid, this shouldnt happen!\r\n");
            return false;
        }
    }
    m_claimed = true;
    return true;
}

int ActivationTriggerList::assignedDevices()
{
    int assigned = 0;
    for (auto &trigger : triggers)
    {
        assigned |= trigger->assignedDevices();
    }
    return assigned;
}

bool ActivationTriggerList::forcedConsoleMode(ConsoleMode& mode) const
{
    bool found = false;
    for (auto &trigger : triggers)
    {
        ConsoleMode trigger_mode;
        if (trigger->forcedConsoleMode(trigger_mode))
        {
            mode = trigger_mode;
            found = true;
        }
    }
    return found;
}
