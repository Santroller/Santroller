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
    uint8_t profile_id;
    std::vector<std::unique_ptr<Mapping>> mappings;
    std::vector<std::unique_ptr<ActivationTrigger>> triggers;
};