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
    virtual void deinitialize() {}
    virtual bool is_bluetooth() const { return false; }
    virtual void process(bool full_poll, bool send_events) = 0;
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
    uint8_t stagekit_param = 0;
    uint8_t stagekit_command = 0;
    uint8_t stagekit_fog = 0;
    uint16_t stagekit_strobe_speed = 0;
    uint8_t stagekit_strobe = 0;
    uint8_t stagekit_blue = 0;
    uint8_t stagekit_green = 0;
    uint8_t stagekit_yellow = 0;
    uint8_t stagekit_red = 0;
    uint32_t stagekit_last_strobe = 0;
    uint8_t capabilities = 0;
    bool side = 0;

    void set_rumble(uint8_t left, uint8_t right);
    void set_player_led(uint8_t player);
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b);
    void set_euphoria_led(uint8_t val);
    void process_stagekit_command(uint8_t command, uint8_t param);
    void update_stagekit();
    void update_feedback(bool force = false);
    void update_capabilities();
};