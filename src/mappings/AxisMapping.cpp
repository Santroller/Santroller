#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include <pb_encode.h>

AxisMapping::AxisMapping(proto_AxisMapping mapping, Input& input, uint16_t id): Mapping(id), m_mapping(mapping), m_input(input) {

}

void AxisMapping::update(san_base_t* base) {
    auto val = m_input.tickAnalog();
    if (val != m_lastValue)
    {
        uint8_t buffer[63];
        proto_Event event = {which_event : proto_AxisEvent_id_tag, event : {axis : {m_id, val}}};
        pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 63);
        pb_encode(&outputStream, proto_Event_fields, &event);
        tud_hid_report(REPORT_ID_CONFIG, buffer, outputStream.bytes_written);
        m_lastValue = val;
    }
}