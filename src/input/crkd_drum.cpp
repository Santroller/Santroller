#include "input/crkd_drum.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

CrkdDrumInput::CrkdDrumInput(proto_CrkdDrumAxisInput input, std::shared_ptr<CrkdDrumDevice> device) : m_input(input), m_device(device)
{
}
bool CrkdDrumInput::tickDigital()
{
    return tickAnalog() > 0;
}
uint16_t CrkdDrumInput::tickAnalog()
{
    switch (m_input.axis)
    {
    case CrkdRedPad:
        return m_device->drum.red_pad << 8;
    case CrkdYellowPad:
        return m_device->drum.yellow_pad << 8;
    case CrkdBluePad:
        return m_device->drum.blue_pad << 8;
    case CrkdGreenPad:
        return m_device->drum.green_pad << 8;
    case CrkdYellowCymbal:
        return m_device->drum.yellow_cymbal << 8;
    case CrkdBlueCymbal:
        return m_device->drum.blue_cymbal << 8;
    case CrkdGreenCymbal:
        return m_device->drum.green_cymbal << 8;
    case CrkdKick1:
        return m_device->drum.kick1 << 8;
    case CrkdKick2:
        return m_device->drum.kick2 << 8;
    }
    return 0;
}
void CrkdDrumInput::setup()
{
}