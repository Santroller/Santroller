#include "input/ads1115.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

ADS1115Input::ADS1115Input(proto_ADS1115Input input, std::shared_ptr<ADS1115Device> device) : m_channel(input.channel), m_input(input), m_device(device)
{
}
void ADS1115Input::setup()
{
}
bool ADS1115Input::tickDigital()
{
    return m_device->ads1115.inputs[m_channel] > 0;
}
uint16_t ADS1115Input::tickAnalog()
{
    return m_device->ads1115.inputs[m_channel];
}