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
    return m_device->tick_analog(m_input.axis) != 0;
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
    return m_device->tick_digital(m_input.button) ? UINT16_MAX : 0;
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
    return 0;
}
uint16_t KeyboardKeyInput::tick_analog()
{
    if (!m_device) return 0;
    return 0;
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
    return 0;
}
uint16_t MouseButtonInput::tick_analog()
{
    if (!m_device) return 0;
    return 0;
}
void MouseButtonInput::setup()
{
}
MouseAxisInput::MouseAxisInput(proto_MouseAxisInput input, std::shared_ptr<UsbHostInterface> device, Profile* profile) : m_input(input), m_device(device), m_profile(profile)
{
}
bool MouseAxisInput::tick_digital()
{
    if (!m_device) return false;
    return 0;
}
uint16_t MouseAxisInput::tick_analog()
{
    if (!m_device) return 0;
    return 0;
}
void MouseAxisInput::setup()
{
}