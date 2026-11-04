#include "input/toggle.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include "events.pb.h"
#include "utils.h"
#include <pb_encode.h>

ToggleInput::ToggleInput() {}
void ToggleInput::load(proto_ToggleInput config,
                       std::shared_ptr<ToggleDevice> device,
                       std::unique_ptr<Input> input)
{
    m_input = std::move(input);
    m_device = device;
    m_last_toggled = 0;
}
void ToggleInput::setup()
{
    if (m_input)
    {
        m_input->setup();
    }
}
bool ToggleInput::tick_digital_raw()
{
    if (!m_device)
    {
        return false;
    }
    if (m_input)
    {
        const bool pressed = m_input->tick_digital();
        if (pressed && !m_last_state)
        {
            m_device->toggle();
        }
        m_last_state = pressed;
    }
    return m_device->get_value();
}
uint16_t ToggleInput::tick_analog()
{
    return tick_digital() ? 65535 : 0;
}