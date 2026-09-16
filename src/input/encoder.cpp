#include "input/encoder.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

EncoderInput::EncoderInput(proto_EncoderInput input, std::shared_ptr<EncoderDevice> device) : m_input(input), m_device(device)
{
}
bool EncoderInput::tick_digital()
{
    return false;
}
uint16_t EncoderInput::tick_analog()
{
    if (m_input.type == proto_EncoderInputType_EncoderDelta)
        return m_device->encoder.delta;
    if (m_input.type == proto_EncoderInputType_EncoderPosition)
        return m_device->encoder.position;
    return 0;
}
void EncoderInput::setup()
{
}