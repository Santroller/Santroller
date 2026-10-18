#include "input/secondary_pico.hpp"

PeripheralInput::PeripheralInput(proto_PeripheralInput input, std::shared_ptr<SecondaryPicoDevice> device, Profile *profile)
    : m_input(input), m_device(device)
{
    if (m_device && m_input.pin >= 0)
    {
        // Read on the secondary pico as a GPIO input of its own
        proto_Input gpio = proto_Input_init_zero;
        gpio.which_input = proto_Input_gpio_tag;
        gpio.input.gpio.pin = m_input.pin;
        gpio.input.gpio.pinMode = m_input.pinMode;
        gpio.input.gpio.analog = m_input.analog;
        m_id = m_device->register_input(gpio);
    }
}

PeripheralInput::~PeripheralInput()
{
    if (m_device && m_id)
    {
        m_device->unregister_input(m_id);
    }
}

bool PeripheralInput::tick_digital()
{
    if (!m_device || !m_id) return false;
    return m_device->read_digital_pin(m_id);
}

uint16_t PeripheralInput::tick_analog()
{
    if (!m_device || !m_id) return 0;
    if (m_input.analog)
    {
        return m_device->read_analog_pin(m_id);
    }
    return m_device->read_digital_pin(m_id) ? 65535 : 0;
}

SecondaryPicoForwardedInput::SecondaryPicoForwardedInput(const proto_Input &input, std::shared_ptr<SecondaryPicoDevice> device, std::unique_ptr<Input> local)
    : m_device(device), m_local(std::move(local))
{
    m_id = m_device->register_input(input);
}

SecondaryPicoForwardedInput::~SecondaryPicoForwardedInput()
{
    if (m_id)
    {
        m_device->unregister_input(m_id);
    }
}

bool SecondaryPicoForwardedInput::tick_digital()
{
    if (!m_id) return false;
    if (has_independent_analog_value())
    {
        // the secondary pico reports these as axes
        return m_device->read_analog_pin(m_id) > 32767;
    }
    return m_device->read_digital_pin(m_id);
}

uint16_t SecondaryPicoForwardedInput::tick_analog()
{
    if (!m_id) return 0;
    if (has_independent_analog_value())
    {
        return m_device->read_analog_pin(m_id);
    }
    return m_device->read_digital_pin(m_id) ? 65535 : 0;
}
