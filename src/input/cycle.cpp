#include "input/cycle.hpp"
#include "utils.h"
#include "config.hpp"
#include <pb_encode.h>
#include "usb/device/hid_device.h"
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
bool CycleInput::tickDigital()
{
    return tickAnalog() > 0;
}
uint16_t CycleInput::tickAnalog()
{
    if (m_input)
    {
        if (m_input->tickDigital() && !m_last_state)
        {
            m_device->cycle(true);
        }
        m_last_state = m_input->tickDigital();
    }
    if (m_input_reverse)
    {
        if (m_input_reverse->tickDigital() && !m_last_state_reverse)
        {
            m_device->cycle(false);
        }
        m_last_state_reverse = m_input_reverse->tickDigital();
    }
    return m_device->get_value();
}