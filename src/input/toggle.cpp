#include "input/toggle.hpp"
#include "utils.h"
#include "config/config.hpp"
#include <pb_encode.h>
#include "emulation/usb/hid_device.h"
#include "events.pb.h"

ToggleInput::ToggleInput()
{
}
void ToggleInput::load(proto_ToggleInput config, std::shared_ptr<ToggleDevice> device, std::unique_ptr<Input> input)
{
    m_input = std::move(input);
    m_device = device;
    m_last_toggled = 0;
}
void ToggleInput::setup()
{
    m_input->setup();
}
bool ToggleInput::tick_digital()
{
     if (m_input)
    {
        if (m_input->tick_digital() && !m_last_state)
        {
            m_device->toggle();
        }
        m_last_state = m_input->tick_digital();
    }
    return m_device->get_value();
}
uint16_t ToggleInput::tick_analog()
{
    return tick_digital() ? 65535 : 0;
}