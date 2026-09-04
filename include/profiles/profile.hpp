#pragma once
#include <map>
#include <vector>
#include <memory>
#include "input.pb.h"
#include "config.pb.h"
#include "devices/base.hpp"
#include "input/input.hpp"
#include "triggers/activation_trigger_list.hpp"

// Forward declarations to avoid circular dependencies
class Mapping;
class LedMapping;

struct ActiveProfileSource
{
    uint16_t device_id;
    uint32_t source_id;
};

class Profile
{
public:
    virtual ~Profile();
    char name[32];
    SubType subtype;
    bool xinput_on_windows;
    bool invert_y_axis_hid;
    bool supports_ps4;
    bool supports_slider;
    bool cymbal_glitch_fix;
    ConsoleMode mode;
    uint32_t profile_id;
    std::vector<std::unique_ptr<Mapping>> mappings;
    std::vector<std::unique_ptr<ActivationTriggerList>> triggers;
    std::vector<std::unique_ptr<LedMapping>> leds;
    std::vector<ActiveProfileSource> activation_sources;
    std::map<uint16_t, std::shared_ptr<Device>> devices;
    DrumState drum_state;
    KeyboardState keyboard_state;
};