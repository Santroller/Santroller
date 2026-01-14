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
    uint8_t rumble_left = 0;
    uint8_t rumble_right = 0;
    uint8_t player_led = 0;
    uint8_t euphoria_led = 0;
    uint8_t lightbar_red = 0;
    uint8_t lightbar_green = 0;
    uint8_t lightbar_blue = 0;
    uint8_t stagekit_command = 0;
    uint8_t stagekit_param = 0;
    bool side = 0;
};