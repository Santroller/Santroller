#include "mappings/rock_revolution_mappings.hpp"

RockRevolutionButtonMapping::RockRevolutionButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

RockRevolutionAxisMapping::RockRevolutionAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, true)
{
}