#include "mappings/djmax_mappings.hpp"

DJMaxTurntableButtonMapping::DJMaxTurntableButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

DJMaxTurntableAxisMapping::DJMaxTurntableAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : AxisMapping(mapping, std::move(input), id, profile, false)
{
}
