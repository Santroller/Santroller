#include "input/infinium_fader.hpp"
#include <memory>

InfiniumFaderInput::InfiniumFaderInput(proto_InfiniumFaderInput input, std::shared_ptr<InfiniumFaderDevice> device, Profile *profile)
    : m_input(input), m_device(device)
{
}

bool InfiniumFaderInput::tick_digital()
{
    return false;
}

uint16_t InfiniumFaderInput::tick_analog()
{
    if (!m_device) return 0;
    return m_device->fader.position;
}
