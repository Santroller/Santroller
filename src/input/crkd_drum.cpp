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
        return m_device->drum.red_pad;
    case CrkdYellowPad:
        return m_device->drum.yellow_pad;
    case CrkdBluePad:
        return m_device->drum.blue_pad;
    case CrkdGreenPad:
        return m_device->drum.green_pad;
    case CrkdYellowCymbal:
        return m_device->drum.yellow_cymbal;
    case CrkdBlueCymbal:
        return m_device->drum.blue_cymbal;
    case CrkdGreenCymbal:
        return m_device->drum.green_cymbal;
    case CrkdKick1:
        return m_device->drum.kick1;
    case CrkdKick2:
        return m_device->drum.kick2;
    }
    return 0;
}
void CrkdDrumInput::setup()
{
}