#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include <pb_encode.h>
#include <utils.h>

AxisMapping::AxisMapping(proto_AxisMapping mapping, std::unique_ptr<Input> input, uint16_t id): Mapping(id), m_mapping(mapping), m_input(std::move(input)) {

}

void AxisMapping::update(san_base_t* base, bool resend_events) {
    // Maybe instead of having a `san_base_t` at all, we just have seperate functions in here that deal with different controller types, and just map everything once.
    auto val = m_input->tickAnalog();
    if (val != m_lastValue || resend_events)
    {
        proto_Event event = {which_event : proto_Event_axis_tag, event : {axis : {m_id, val}}};
        send_event(event, resend_events);
        m_lastValue = val;
    }
    if (base == nullptr) {
        return;
    }
}