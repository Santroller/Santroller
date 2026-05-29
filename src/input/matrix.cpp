#include "input/matrix.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

MatrixInput::MatrixInput(proto_MatrixInput input, std::shared_ptr<MatrixDevice> device) : m_input(input), m_device(device)
{
}
bool MatrixInput::tickDigital()
{
    if (m_input.pin == -1 || m_input.outputPin == -1)
        return false;
    return m_device->read_pin(m_input.pin, m_input.outputPin);
}
uint16_t MatrixInput::tickAnalog()
{
    return tickDigital() ? UINT16_MAX : 0;
}
void MatrixInput::setup()
{
}