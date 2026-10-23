#include "triggers/activation_trigger_list.hpp"
#include "triggers/activation_trigger.hpp"
#include "mappings/mapping.hpp"
#include "tusb.h"
#include "emulation/usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include <algorithm>

// Destructor must be defined where ActivationTrigger is a complete type
ActivationTriggerList::~ActivationTriggerList() = default;

void send_activation_list_event(uint32_t list_id, bool state)
{
    proto_Event event = {which_event : proto_Event_activationList_tag, event : {activationList : {list_id, state}}};
    HIDConfigDevice::send_event(event, true);
}

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
    ActivationTrigger *auth = nullptr;
    for (auto &trigger : triggers)
    {
        if (trigger->holds_xbox_one_auth())
        {
            auth = trigger.get();
        }
    }
    bool auth_matched = auth && auth->validate(false, full_poll, send_events);
    bool claimable = true;
    bool held = false;
    bool matched = validate_triggers(claim_devices, full_poll, send_events, false, claimable);
    // The controllers can be swapped out while Xbox One auth holds the list up, as the console won't ask again
    if (!matched && auth_matched)
    {
        matched = validate_triggers(claim_devices, full_poll, send_events, true, claimable);
        held = matched;
    }
    if (!claim_devices)
    {
        if (send_events && (matched != m_last_val || full_poll))
        {
            send_activation_list_event(list_id, matched);
        }
        m_last_val = matched;
        return matched && claimable;
    }
    if (matched)
    {
        m_claimed = true;
        m_held = held;
        // Makes this the rule that stays matched once the controller authenticating for it is unplugged
        if (auth)
        {
            auth->validate(true, full_poll, send_events);
        }
    }
    return matched;
}

// held skips the controllers, and checks the rest of the list. claimable is cleared when a controller
// it matched is claimed by another profile
bool ActivationTriggerList::validate_triggers(bool claim_devices, bool full_poll, bool send_events, bool held, bool &claimable)
{
    auto skipped = [held](const std::unique_ptr<ActivationTrigger> &trigger)
    {
        return trigger->holds_xbox_one_auth() || (held && trigger->claims_device());
    };
    // A held button is only seen once its device sends a report, which can be after a profile
    // further down claimed the device. Reading it anyway lets the startup trigger latch and reload,
    // and the reload then picks the profiles in order again
    bool reads_claimed = false;
    if (!claim_devices)
    {
        for (auto &trigger : triggers)
        {
            reads_claimed |= trigger->awaiting_startup_input();
        }
    }
    bool matched = true;
    claimable = true;
    for (auto &trigger : triggers)
    {
        if (skipped(trigger))
        {
            continue;
        }
        trigger->set_reads_claimed(reads_claimed);
        if (!trigger->validate(false, full_poll, send_events))
        {
            matched = false;
            if (claim_devices)
            {
                break;
            }
        }
        if (trigger->matched_claimed())
        {
            claimable = false;
        }
        trigger->set_reads_claimed(false);
    }
    for (auto &trigger2 : triggers)
    {
        trigger2->reset();
    }
    if (!claim_devices || !matched)
    {
        return matched;
    }
    for (auto &trigger : triggers)
    {
        if (!skipped(trigger) && !trigger->validate(true, full_poll, send_events))
        {
            printf("trigger was not valid, this shouldnt happen!\r\n");
            return false;
        }
    }
    return true;
}

void ActivationTriggerList::order_triggers()
{
    // An input can read a device that another trigger of the list puts in a slot (eg a button on
    // the guitar a USB type trigger picked), which only works once that trigger has been checked
    std::stable_partition(triggers.begin(), triggers.end(), [](const auto &trigger)
                          { return !trigger->reads_input(); });
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

bool ActivationTriggerList::xinputOnWindows(bool &enabled) const
{
    bool found = false;
    for (auto &trigger : triggers)
    {
        bool trigger_val;
        if (trigger->xinputOnWindows(trigger_val))
        {
            enabled = trigger_val;
            found = true;
        }
    }
    return found;
}

bool ActivationTriggerList::ps4OrPs5Mode(bool &enabled) const
{
    bool found = false;
    for (auto &trigger : triggers)
    {
        bool trigger_val;
        if (trigger->ps4OrPs5Mode(trigger_val))
        {
            enabled = trigger_val;
            found = true;
        }
    }
    return found;
}
