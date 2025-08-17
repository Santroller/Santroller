#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>

AxisMapping::AxisMapping(proto_AxisMapping mapping, std::unique_ptr<Input> input, uint16_t id): Mapping(id), m_mapping(mapping), m_input(std::move(input)) {

}

void AxisMapping::update(bool full_poll) {
    auto val = m_input->tickAnalog();
    if (val != m_lastValue || full_poll)
    {
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, val}}};
        send_event(event);
        m_lastValue = val;
    }
}