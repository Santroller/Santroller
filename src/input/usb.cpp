#include "input/usb.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

USBAxisInput::USBAxisInput(proto_USBAxisInput input, std::shared_ptr<UsbHostInterface> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool USBAxisInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->tick_axis_digital(m_input.axis);
}
uint16_t USBAxisInput::tick_analog()
{
    if (!m_device) return 0;
    return m_device->tick_analog(m_input.axis);
}
void USBAxisInput::setup()
{
}
USBButtonInput::USBButtonInput(proto_USBButtonInput input, std::shared_ptr<UsbHostInterface> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool USBButtonInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->tick_digital(m_input.button);
}
uint16_t USBButtonInput::tick_analog()
{
    if (!m_device) return 0;
    return m_device->tick_button_pressure(m_input.button);
}
bool USBButtonInput::tick_pro_key_range(uint32_t &active_keys, uint8_t *velocities, uint8_t key_count)
{
    if (!m_device || m_input.button.which_mapping != proto_Output_proKeyMultiple_tag)
    {
        return false;
    }

    if (key_count > 25)
    {
        key_count = 25;
    }
    if (m_input.button.mapping.proKeyMultiple < 0)
    {
        key_count = 0;
    }
    else if (key_count > m_input.button.mapping.proKeyMultiple)
    {
        key_count = static_cast<uint8_t>(m_input.button.mapping.proKeyMultiple);
    }
    active_keys = 0;
    for (uint8_t i = 0; i < key_count; ++i)
    {
        proto_Output key_output = {};
        key_output.which_mapping = proto_Output_proKeySingle_tag;
        key_output.mapping.proKeySingle = i + 1;
        if (m_device->tick_digital(key_output))
        {
            active_keys |= uint32_t(1) << i;
            velocities[i] = static_cast<uint8_t>(m_device->tick_button_pressure(key_output) >> 9);
        }
        else
        {
            velocities[i] = 0;
        }
    }
    return true;
}
void USBButtonInput::setup()
{
}
KeyboardKeyInput::KeyboardKeyInput(proto_KeyboardKeyInput input, std::shared_ptr<UsbHostInterface> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool KeyboardKeyInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->key_pressed(m_input.key);
}
uint16_t KeyboardKeyInput::tick_analog()
{
    return tick_digital() ? UINT16_MAX : 0;
}
void KeyboardKeyInput::setup()
{
}
MouseButtonInput::MouseButtonInput(proto_MouseButtonInput input, std::shared_ptr<UsbHostInterface> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool MouseButtonInput::tick_digital()
{
    if (!m_device) return false;
    return m_device->mouse_button(m_input.button);
}
uint16_t MouseButtonInput::tick_analog()
{
    return tick_digital() ? UINT16_MAX : 0;
}
void MouseButtonInput::setup()
{
}
MouseAxisInput::MouseAxisInput(proto_MouseAxisInput input, std::shared_ptr<UsbHostInterface> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool MouseAxisInput::tick_digital()
{
    return tick_analog() != UINT16_MAX / 2;
}
uint16_t MouseAxisInput::tick_analog()
{
    if (!m_device) return UINT16_MAX / 2;
    return m_device->mouse_axis(m_input.axis);
}
void MouseAxisInput::setup()
{
}