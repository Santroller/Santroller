#include "input/switch_network.hpp"

SwitchNetworkInput::SwitchNetworkInput(proto_SwitchNetworkInput input, std::shared_ptr<SwitchNetworkDevice> device, Profile* profile)
    : m_input(input), m_device(device)
{
    (void)profile;
    setup();
}

bool SwitchNetworkInput::tick_digital()
{
    if (!m_device || m_input.pin < 0 || m_input.otherPin < 0)
        return false;

    return m_device->read_switch(
        static_cast<uint8_t>(m_input.pin),
        static_cast<uint8_t>(m_input.otherPin));
}

uint16_t SwitchNetworkInput::tick_analog()
{
    return tick_digital() ? UINT16_MAX : 0;
}

void SwitchNetworkInput::setup()
{
}
