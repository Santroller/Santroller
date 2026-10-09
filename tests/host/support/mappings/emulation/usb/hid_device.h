#pragma once
// Fake HID config device: events the mappings send to the config tool are recorded for tests
#include <vector>
#include "events.pb.h"

class HIDConfigDevice
{
public:
    static inline std::vector<proto_Event> sent_events;
    static void send_event(proto_Event event, bool force)
    {
        (void)force;
        sent_events.push_back(event);
    }
};
