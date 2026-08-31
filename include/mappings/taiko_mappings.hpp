#pragma once

#include "mappings/base_mapping.hpp"

class TaikoButtonMapping : public ButtonMapping
{
public:
    TaikoButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
};

class TaikoAxisMapping : public AxisMapping
{
public:
    TaikoAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
};