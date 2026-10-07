#include "input/multiplexer.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

MultiplexerInput::MultiplexerInput(proto_MultiplexerInput input, std::shared_ptr<MultiplexerDevice> device, Profile* profile) : m_channel(input.channel), m_input(input), m_device(device)
{
}
void MultiplexerInput::setup()
{
}
bool MultiplexerInput::tick_digital()
{
    // the ADC is never exactly 0, so treat it as pressed past half way
    return m_device->read(m_channel) > UINT16_MAX / 2;
}
uint16_t MultiplexerInput::tick_analog()
{
    return m_device->read(m_channel);
}