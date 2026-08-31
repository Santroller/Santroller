#pragma once

#include "mappings/base_mapping.hpp"

class DJMaxTurntableButtonMapping : public ButtonMapping
{
public:
    DJMaxTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
};

class DJMaxTurntableAxisMapping : public AxisMapping
{
public:
    DJMaxTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
};