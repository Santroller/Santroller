#include "input/crazy_guitar_neck.hpp"

CrazyGuitarNeckButtonInput::CrazyGuitarNeckButtonInput(proto_CrazyGuitarNeckButtonInput input, std::shared_ptr<CrazyGuitarNeckDevice> device, Profile* profile) : m_input(input), m_device(device)
{
}
bool CrazyGuitarNeckButtonInput::tick_digital()
{
    const auto &neck = m_device->m_crazy_guitar_neck;
    switch (m_input.button)
    {
    case CrazyGuitarNeckGreen:
        return neck.green;
    case CrazyGuitarNeckRed:
        return neck.red;
    case CrazyGuitarNeckYellow:
        return neck.yellow;
    case CrazyGuitarNeckBlue:
        return neck.blue;
    case CrazyGuitarNeckOrange:
        return neck.orange;
    case CrazyGuitarNeckSoloGreen:
        return neck.soloGreen;
    case CrazyGuitarNeckSoloRed:
        return neck.soloRed;
    case CrazyGuitarNeckSoloYellow:
        return neck.soloYellow;
    case CrazyGuitarNeckSoloBlue:
        return neck.soloBlue;
    case CrazyGuitarNeckSoloOrange:
        return neck.soloOrange;
    }
    return false;
}
uint16_t CrazyGuitarNeckButtonInput::tick_analog()
{
    return tick_digital() ? 32767 : 0;
}
void CrazyGuitarNeckButtonInput::setup()
{
}
