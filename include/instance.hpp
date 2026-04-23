#pragma once
#include <map>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"
#include "input/gpio.hpp"
#include "leds/leds.hpp"

#include "config.pb.h"
#include <stdio.h>


class ActivationTriggerList
{
public:
    uint32_t profile_id;
    std::vector<std::unique_ptr<ActivationTrigger>> triggers;
    bool validate(bool claim_devices, bool full_poll, bool send_events);
    int assignedDevices();
private:
    bool m_claimed = false;
};

class Profile
{
public:
    virtual ~Profile() {}
    char name[32];
    SubType subtype;
    bool xinput_on_windows;
    bool invert_y_axis_hid;
    bool supports_ps4;
    ConsoleMode mode;
    uint32_t profile_id;
    std::vector<std::unique_ptr<Mapping>> mappings;
    std::vector<std::unique_ptr<ActivationTriggerList>> triggers;
    std::vector<std::unique_ptr<LedMapping>> leds;
    std::map<uint16_t, std::shared_ptr<Device>> devices;
    std::map<uint16_t, std::shared_ptr<Device>> midiDevices;
};
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