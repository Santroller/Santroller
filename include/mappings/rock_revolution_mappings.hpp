#pragma once

#include "mappings/base_mapping.hpp"

class RockRevolutionButtonMapping : public ButtonMapping
{
public:
    RockRevolutionButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
};

class RockRevolutionAxisMapping : public AxisMapping
{
public:
    RockRevolutionAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
};