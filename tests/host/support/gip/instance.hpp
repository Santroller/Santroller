#pragma once
// Fake instance.hpp: the parts of Instance the emulated Xbox One device uses. Rumble and player
// LED requests are recorded so tests can see what the console asked for.
#include <memory>
#include <vector>
#include "config.pb.h"
#include "profiles/profile.hpp"

class Instance
{
public:
    virtual ~Instance() {}
    virtual void initialize() = 0;
    virtual void process(bool full_poll, bool send_events) = 0;
    SubType subtype = Gamepad;
    ConsoleMode mode = ModeXboxOne;
    std::vector<std::shared_ptr<Profile>> profiles;
    uint8_t rumble_left = 0;
    uint8_t rumble_right = 0;
    uint8_t player_led = 1;
    int rumble_calls = 0;
    int player_led_calls = 0;
    void set_rumble(uint8_t left, uint8_t right)
    {
        rumble_left = left;
        rumble_right = right;
        rumble_calls++;
    }
    void set_player_led(uint8_t player)
    {
        player_led = player;
        player_led_calls++;
    }
};
