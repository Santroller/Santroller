#pragma once
// Fake instance.hpp: just the state the device triggers copy onto a claimed device
#include <stdint.h>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "profiles/profile.hpp"

class Instance
{
public:
    virtual ~Instance() {}
    uint8_t rumble_left = 0;
    uint8_t rumble_right = 0;
    uint8_t player_led = 1;
    uint8_t euphoria_led = 0;
    uint8_t lightbar_red = 0;
    uint8_t lightbar_green = 0;
    uint8_t lightbar_blue = 0;
    int capability_updates = 0;
    void update_capabilities() { capability_updates++; }
};
