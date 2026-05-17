#include "input/protarNeck.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

ProtarNeckAxisInput::ProtarNeckAxisInput(proto_ProtarNeckAxisInput input, std::shared_ptr<ProtarNeckDevice> device) : m_input(input), m_device(device)
{
}
bool ProtarNeckAxisInput::tickDigital()
{
    return m_device->readAxis(m_input.axis) != 0;
}
uint16_t ProtarNeckAxisInput::tickAnalog()
{
    return m_device->readAxis(m_input.axis);
}
void ProtarNeckAxisInput::setup()
{
}

ProtarNeckButtonInput::ProtarNeckButtonInput(proto_ProtarNeckButtonInput input, std::shared_ptr<ProtarNeckDevice> device) : m_input(input), m_device(device)
{
}
bool ProtarNeckButtonInput::tickDigital()
{
    return m_device->readButton(m_input.button);
}
uint16_t ProtarNeckButtonInput::tickAnalog()
{
    return tickDigital() ? 65535 : 0;
}
void ProtarNeckButtonInput::setup()
{
}