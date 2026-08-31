#pragma once

#include "mappings/base_mapping.hpp"

class PowerGigGuitarButtonMapping : public ButtonMapping
{
public:
    PowerGigGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

class PowerGigGuitarAxisMapping : public AxisMapping
{
public:
    PowerGigGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

class PowerGigDrumsButtonMapping : public ButtonMapping
{
public:
    PowerGigDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

class PowerGigDrumsAxisMapping : public AxisMapping
{
public:
    PowerGigDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};