#include "input/bt.hpp"

BTButtonInput::BTButtonInput(proto_BTButtonInput input,
                             std::shared_ptr<BluetoothHostInterface> device)
    : m_input(input), m_device(device)
{
}

bool BTButtonInput::tick_digital()
{
    return m_device->tick_digital(m_input.button);
}

uint16_t BTButtonInput::tick_analog()
{
    return m_device->tick_digital(m_input.button) ? UINT16_MAX : 0;
}

void BTButtonInput::setup()
{
}

BTAxisInput::BTAxisInput(proto_BTAxisInput input,
                         std::shared_ptr<BluetoothHostInterface> device)
    : m_input(input), m_device(device)
{
}

bool BTAxisInput::tick_digital()
{
    return m_device->tick_analog(m_input.axis) != 0;
}

uint16_t BTAxisInput::tick_analog()
{
    return m_device->tick_analog(m_input.axis);
}

void BTAxisInput::setup()
{
}

