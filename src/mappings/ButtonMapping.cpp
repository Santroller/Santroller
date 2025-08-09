#include "mappings/mapping.hpp"

ButtonMapping::ButtonMapping(proto_ButtonMapping mapping, Input& input): m_mapping(mapping), m_input(input) {

}

void ButtonMapping::update(san_base_t *base) {
    switch (m_mapping.button) {
        case proto_ButtonType::North:
            base->gamepad.a = m_input.tickDigital();
            break;
    }
}