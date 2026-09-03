#include "input/protar_neck.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

ProtarNeckAxisInput::ProtarNeckAxisInput(proto_ProtarNeckAxisInput input, std::shared_ptr<ProtarNeckDevice> device) : m_input(input), m_device(device)
{
}
bool ProtarNeckAxisInput::tick_digital()
{
    return m_device->read_axis(m_input.axis) != 0;
}
uint16_t ProtarNeckAxisInput::tick_analog()
{
    return m_device->read_axis(m_input.axis);
}
void ProtarNeckAxisInput::setup()
{
}

ProtarNeckButtonInput::ProtarNeckButtonInput(proto_ProtarNeckButtonInput input, std::shared_ptr<ProtarNeckDevice> device) : m_input(input), m_device(device)
{
}
bool ProtarNeckButtonInput::tick_digital()
{
    return m_device->read_button(m_input.button);
}
uint16_t ProtarNeckButtonInput::tick_analog()
{
    return tick_digital() ? 65535 : 0;
}
void ProtarNeckButtonInput::setup()
{
}