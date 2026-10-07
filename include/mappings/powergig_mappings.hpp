#pragma once

#include "mappings/base_mapping.hpp"
#include "mappings/rock_band_mappings.hpp"

// The PowerGig guitar is a Rock Band guitar everywhere except its own PS3 report
class PowerGigGuitarButtonMapping : public RockBandGuitarButtonMapping
{
public:
    PowerGigGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

class PowerGigGuitarAxisMapping : public RockBandGuitarAxisMapping
{
public:
    PowerGigGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

// Likewise, the PowerGig drums are Rock Band drums everywhere except their own PS3 report
class PowerGigDrumsButtonMapping : public RockBandDrumsButtonMapping
{
public:
    PowerGigDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

class PowerGigDrumsAxisMapping : public RockBandDrumsAxisMapping
{
public:
    PowerGigDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};