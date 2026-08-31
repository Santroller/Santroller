#pragma once
#include <map>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"
#include "input/gpio.hpp"
#include "input/input.hpp"
#include "leds/leds.hpp"
#include "profiles/profile.hpp"

#include "config.pb.h"
#include <stdio.h>

class Instance
{
public:
    virtual ~Instance() {}
    virtual void initialize() = 0;
    virtual void process() = 0;
    SubType subtype;
    ConsoleMode mode;
    bool xinput_on_windows = 0;
    bool invert_y_axis_hid = 0;
    bool supports_ps4 = 0;
    std::vector<std::shared_ptr<Profile>> profiles;
    uint8_t rumble_left = 0;
    uint8_t rumble_right = 0;
    uint8_t player_led = 0;
    uint8_t euphoria_led = 0;
    uint8_t lightbar_red = 0;
    uint8_t lightbar_green = 0;
    uint8_t lightbar_blue = 0;
    uint8_t stagekit_command = 0;
    uint8_t stagekit_param = 0;
    uint8_t capabilities = 0;
    bool side = 0;
};