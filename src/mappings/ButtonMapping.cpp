#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"

ButtonMapping::ButtonMapping(proto_ButtonMapping mapping, std::unique_ptr<Input> input, uint16_t id) : Mapping(id), m_mapping(mapping), m_input(std::move(input))
{
}

void ButtonMapping::update(san_base_t *base, bool resend_events)
{
    auto val = m_input->tickDigital();
    if (val != m_lastValue || resend_events)
    {
        proto_Event event = {which_event : proto_Event_button_tag, event : {button : {m_id, val}}};
        send_event(event);
        m_lastValue = val;
    }
    switch (m_mapping.button)
    {
    case proto_ButtonType::A:
        base->gamepad.a = val;
        break;
    }
}