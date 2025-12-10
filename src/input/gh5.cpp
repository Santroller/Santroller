#include "input/gh5.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

Gh5ButtonInput::Gh5ButtonInput(proto_Gh5NeckButtonInput input, std::shared_ptr<GH5NeckDevice> device) : m_input(input), m_device(device)
{
}
bool Gh5ButtonInput::tickDigital()
{
    switch (m_input.button)
    {
    case Gh5Green:
        return m_device->green;
    case Gh5Red:
        return m_device->red;
    case Gh5Yellow:
        return m_device->yellow;
    case Gh5Blue:
        return m_device->blue;
    case Gh5Orange:
        return m_device->orange;
    case Gh5TapGreen:
        return m_device->tapGreen;
    case Gh5TapRed:
        return m_device->tapRed;
    case Gh5TapYellow:
        return m_device->tapYellow;
    case Gh5TapBlue:
        return m_device->tapBlue;
    case Gh5TapOrange:
        return m_device->tapOrange;
    }
    return false;
}
uint16_t Gh5ButtonInput::tickAnalog()
{
    return tickDigital() ? 32767 : 0;
}
void Gh5ButtonInput::setup()
{
}