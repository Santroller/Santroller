#include "input/cycle.hpp"
#include "utils.h"
#include "config/config.hpp"
#include <pb_encode.h>
#include "emulation/usb/hid_device.h"
#include "events.pb.h"

CycleInput::CycleInput()
{
}
void CycleInput::load(proto_CycleInput config, std::shared_ptr<CycleDevice> device, std::unique_ptr<Input> input, std::unique_ptr<Input> input_reverse)
{
    m_input = std::move(input);
    m_input_reverse = std::move(input_reverse);
    m_device = device;
    m_last_toggled = 0;
}
void CycleInput::setup()
{
    m_input->setup();
}
bool CycleInput::tick_digital()
{
    return tick_analog() > 0;
}
uint16_t CycleInput::tick_analog()
{
    if (m_input)
    {
        if (m_input->tick_digital() && !m_last_state)
        {
            m_device->cycle(true);
        }
        m_last_state = m_input->tick_digital();
    }
    if (m_input_reverse)
    {
        if (m_input_reverse->tick_digital() && !m_last_state_reverse)
        {
            m_device->cycle(false);
        }
        m_last_state_reverse = m_input_reverse->tick_digital();
    }
    return m_device->get_value();
}