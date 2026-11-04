#pragma once
// Fake HID config device: events the mappings send to the config tool are recorded for tests
#include <vector>
#include "events.pb.h"

class HIDConfigDevice
{
public:
    static inline std::vector<proto_Event> sent_events;
    // Makes non-forced events find the queue full, like the real device's 4 event list
    static inline bool queue_full = false;
    static bool send_event(proto_Event event, bool force)
    {
        if (queue_full && !force)
        {
            return false;
        }
        sent_events.push_back(event);
        return true;
    }
    // Whether the config tool looks open, which defers modes being forced on the device
    static inline bool tool_open = false;
    static bool tool_may_be_open() { return tool_open; }
};
