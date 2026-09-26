#include "input/bt.hpp"

BTButtonInput::BTButtonInput(proto_BTButtonInput input,
                             std::shared_ptr<BluetoothHostInterface> device, Profile* profile)
    : m_input(input), m_device(device), m_profile(profile)
{
}

bool BTButtonInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->tick_digital(m_input.button);
}

uint16_t BTButtonInput::tick_analog()
{
    if (!m_device) return 0;
    return m_device->tick_digital(m_input.button) ? UINT16_MAX : 0;
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
    return m_device->tick_analog(m_input.axis) != 0;
}

uint16_t BTAxisInput::tick_analog()
{
    if (!m_device) return 0;
    return m_device->tick_analog(m_input.axis);
}

void BTAxisInput::setup()
{
}

