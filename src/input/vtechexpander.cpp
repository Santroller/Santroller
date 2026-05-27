#include "input/vtechexpander.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

VTechExpanderInput::VTechExpanderInput(proto_VTechGuitarIOExpanderInput input, std::shared_ptr<VTechGuitarIOExpanderDevice> device) : m_input(input), m_device(device)
{
}
bool VTechExpanderInput::tickDigital()
{
    return m_device->read_button(m_input.button);
}
uint16_t VTechExpanderInput::tickAnalog()
{
    return tickDigital() ? UINT16_MAX : 0;
}
void VTechExpanderInput::setup()
{
}