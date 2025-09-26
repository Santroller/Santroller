#include "input/crkd.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

CrkdButtonInput::CrkdButtonInput(proto_CrkdNeckButtonInput input, std::shared_ptr<CrkdDevice> device) : m_input(input), m_device(device)
{
}
bool CrkdButtonInput::tickDigital()
{
    switch (m_input.button)
    {
    case CrkdGreen:
        return m_device->neck.green;
    case CrkdRed:
        return m_device->neck.red;
    case CrkdYellow:
        return m_device->neck.yellow;
    case CrkdBlue:
        return m_device->neck.blue;
    case CrkdOrange:
        return m_device->neck.orange;
    case CrkdDpadUp:
        return m_device->neck.dpadUp;
    case CrkdDpadDown:
        return m_device->neck.dpadDown;
    case CrkdDpadLeft:
        return m_device->neck.dpadLeft;
    case CrkdDpadRight:
        return m_device->neck.dpadRight;
    }
    return false;
}
uint16_t CrkdButtonInput::tickAnalog()
{
    return tickDigital() ? 32767 : 0;
}
void CrkdButtonInput::setup()
{
}