#include <map>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"

#include "config.pb.h"

class Instance
{
public:
    virtual void initialize() = 0;
    virtual void process(bool full_poll) = 0;
    SubType subtype;
    ConsoleMode mode;
    uint32_t profile_id;
    std::vector<std::unique_ptr<Mapping>> mappings;
    std::vector<std::unique_ptr<ActivationTrigger>> triggers;
    uint8_t rumble_left;
    uint8_t rumble_right;
    uint8_t player_led;
    uint8_t euphoria_led;
    uint8_t lightbar_red;
    uint8_t lightbar_green;
    uint8_t lightbar_blue;
    uint8_t stagekit_command;
    uint8_t stagekit_param;
    bool side;
};