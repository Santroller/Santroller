#include "input/multiplexer.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

MultiplexerInput::MultiplexerInput(proto_MultiplexerInput input, std::shared_ptr<MultiplexerDevice> device) : m_channel(input.channel), m_input(input), m_device(device)
{
}
void MultiplexerInput::setup()
{
}
bool MultiplexerInput::tickDigital()
{
    return m_device->read(m_channel) > 0;
}
uint16_t MultiplexerInput::tickAnalog()
{
    return m_device->read(m_channel);
}