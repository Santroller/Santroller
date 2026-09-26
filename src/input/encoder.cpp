#include "input/encoder.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

EncoderInput::EncoderInput(proto_EncoderInput input, std::shared_ptr<EncoderDevice> device, Profile* profile) : m_input(input), m_device(device)
{
}
bool EncoderInput::tick_digital()
{
    return false;
}
uint16_t EncoderInput::tick_analog()
{
    if (m_input.type == EncoderDelta)
    {
        int32_t val = (int32_t)m_device->encoder.delta + 32768;
        if (val < 0)
            return 0;
        if (val > UINT16_MAX)
            return UINT16_MAX;
        return static_cast<uint16_t>(val);
    }
    if (m_input.type == EncoderPosition)
        return m_device->encoder.position;
    return 0;
}
void EncoderInput::setup()
{
}