#include "input/bt.hpp"
#include "protocols/face_buttons.hpp"

BTButtonInput::BTButtonInput(proto_BTButtonInput input,
                             std::shared_ptr<BluetoothHostInterface> device, Profile* profile)
    : m_input(input), m_device(device), m_profile(profile)
{
}

// In position based mode the mapped button is a position, so read whichever legend sits there on the pad
bool BTButtonInput::by_position() const
{
    return m_profile && m_profile->position_based_face_buttons && m_device->nintendo_face_layout();
}

bool BTButtonInput::tick_digital()
{
    if (!m_device) return false;
    if (by_position())
    {
        proto_Output button = swap_nintendo_face_button(m_input.button);
        return m_device->tick_digital(button);
    }
    return m_device->tick_digital(m_input.button);
}

uint16_t BTButtonInput::tick_analog()
{
    if (!m_device) return 0;
    if (by_position())
    {
        proto_Output button = swap_nintendo_face_button(m_input.button);
        return m_device->tick_button_pressure(button);
    }
    return m_device->tick_button_pressure(m_input.button);
}

void BTButtonInput::setup()
{
}

BTAxisInput::BTAxisInput(proto_BTAxisInput input,
                         std::shared_ptr<BluetoothHostInterface> device, Profile* profile)
    : m_input(input), m_device(device), m_profile(profile)
{
}

bool BTAxisInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->tick_axis_digital(m_input.axis);
}

uint16_t BTAxisInput::tick_analog()
{
    if (!m_device) return 0;
    return m_device->tick_analog(m_input.axis);
}

void BTAxisInput::setup()
{
}
