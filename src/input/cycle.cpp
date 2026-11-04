#include "input/cycle.hpp"
#include "config/config.hpp"
#include "emulation/usb/hid_device.h"
#include "events.pb.h"
#include "utils.h"
#include <pb_encode.h>

CycleInput::CycleInput() {}
void CycleInput::load(proto_CycleInput, std::shared_ptr<CycleDevice> device,
                      std::unique_ptr<Input> input,
                      std::unique_ptr<Input> input_reverse)
{
    m_input = std::move(input);
    m_input_reverse = std::move(input_reverse);
    m_device = device;
    m_last_toggled = 0;
    // Carry on from what the device last saw, see CycleDevice::input_held
    m_last_state = device->input_held(true);
    m_last_state_reverse = device->input_held(false);
}
void CycleInput::setup()
{
    if (m_input)
    {
        m_input->setup();
    }
    if (m_input_reverse)
    {
        m_input_reverse->setup();
    }
}
bool CycleInput::tick_digital_raw()
{
    auto val = tick_analog();
    return val > 0;
}
uint16_t CycleInput::tick_analog()
{
    // Left unloaded when its cycle device isn't in the profile, see load_cycle
    if (!m_device)
    {
        return 0;
    }
    if (m_input)
    {
        const bool pressed = m_input->tick_digital();
        if (pressed && !m_last_state)
        {
            m_device->cycle(true);
        }
        m_last_state = pressed;
        m_device->set_input_held(true, m_last_state);
    }
    if (m_input_reverse)
    {
        const bool pressed = m_input_reverse->tick_digital();
        if (pressed && !m_last_state_reverse)
        {
            m_device->cycle(false);
        }
        m_last_state_reverse = pressed;
        m_device->set_input_held(false, m_last_state_reverse);
    }
    return m_device->get_value();
}