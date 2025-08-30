#include "mappings/mapping.hpp"
#include "tusb.h"
#include "usb/usb_descriptors.h"
#include "events.pb.h"
#include "main.hpp"
#include "config.hpp"

ActivationTrigger::ActivationTrigger(proto_ActivationTrigger activation_trigger, std::unique_ptr<Input> input, uint16_t profile_id) : m_activation_trigger(activation_trigger), m_input(std::move(input)), m_profile_id(profile_id)
{
}

// deal with debounce and all the other fun things
void ActivationTrigger::update(bool full_poll)
{
    auto val = m_input->tickDigital();
    if (val) {
        set_current_profile(m_profile_id);
    }
}