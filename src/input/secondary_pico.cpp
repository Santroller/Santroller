#include "input/secondary_pico.hpp"

PeripheralInput::PeripheralInput(proto_PeripheralInput input, std::shared_ptr<SecondaryPicoDevice> device, Profile *profile)
    : m_input(input), m_device(device)
{
}

bool PeripheralInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->read_digital_pin(m_input.pin);
}

uint16_t PeripheralInput::tick_analog()
{
    if (!m_device) return 0;
    if (m_input.analog)
    {
        return m_device->read_analog_pin(m_input.pin);
    }
    return m_device->read_digital_pin(m_input.pin) ? 65535 : 0;
}
