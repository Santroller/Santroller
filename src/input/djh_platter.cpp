#include "input/djh_platter.hpp"

DJHeroPlatterInput::DJHeroPlatterInput(proto_DJHeroPlatterInput input, std::shared_ptr<DjHeroTurntableDevice> device, Profile* profile) : m_input(input), m_device(device)
{
}
bool DJHeroPlatterInput::tick_digital()
{
    const auto &platter = m_device->m_turntable;
    switch (m_input.type)
    {
    case DJHeroPlatterGreen:
        return platter.green;
    case DJHeroPlatterRed:
        return platter.red;
    case DJHeroPlatterBlue:
        return platter.blue;
    case DJHeroPlatterVelocity:
        return platter.velocity != 0;
    }
    return false;
}
uint16_t DJHeroPlatterInput::tick_analog()
{
    if (m_input.type == DJHeroPlatterVelocity)
    {
        // the signed reading spans the whole axis, centered
        return (m_device->m_turntable.velocity + 128) << 8;
    }
    return tick_digital() ? 32767 : 0;
}
void DJHeroPlatterInput::setup()
{
}
