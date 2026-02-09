#include "input/mpr121.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

MPR121Input::MPR121Input(proto_MPR121Input input, std::shared_ptr<MPR121Device> device) : m_input(input), m_device(device)
{
}
void MPR121Input::setup()
{
    // TODO: set up DDR and pin counts and things based on the config
}
bool MPR121Input::tickDigital()
{
    return m_device->m_mpr121.inputs & 1 << m_input.pin;
}
uint16_t MPR121Input::tickAnalog()
{
    return tickDigital() ? 65535 : 0;
}