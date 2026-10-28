#pragma once
// Fake profiles/profile.hpp: a profile is just a subtype plus mappings that write a fixed
// report. xone_device.cpp calls update() and report writers on mappings and update() on LEDs.
#include <stdint.h>
#include <string.h>
#include <memory>
#include <optional>
#include <vector>
#include "enums.pb.h"

class FakeXboxOneMapping
{
public:
    // Bytes OR'd into the Xbox One report at the given offset by update_xboxone
    std::vector<uint8_t> bytes;
    size_t offset = 0;
    size_t ps3_writes = 0;
    std::optional<uint8_t> strum;
    void update(bool full_poll, bool send_events)
    {
        (void)full_poll;
        (void)send_events;
    }
    void update_xboxone(uint8_t *report)
    {
        for (size_t i = 0; i < bytes.size(); i++)
            report[offset + i] |= bytes[i];
        if (strum)
            report[4] = *strum;
    }
    void update_ps3(uint8_t *report)
    {
        ps3_writes++;
        update_xboxone(report);
    }
};

class FakeLed
{
public:
    void update(bool full_poll, bool send_events)
    {
        (void)full_poll;
        (void)send_events;
    }
};

class Profile
{
public:
    SubType subtype = Gamepad;
    bool xbox_one_auth_held = false;
    std::vector<std::shared_ptr<FakeXboxOneMapping>> mappings;
    std::vector<std::shared_ptr<FakeLed>> leds;
};
