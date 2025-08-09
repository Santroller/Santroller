#include "mappings/mapping.hpp"

AxisMapping::AxisMapping(proto_AxisMapping mapping, Input& input, uint16_t id): Mapping(id), m_mapping(mapping), m_input(input) {

}

void AxisMapping::update(san_base_t* base) {

}