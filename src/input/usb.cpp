#include "input/usb.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

USBAxisInput::USBAxisInput(proto_USBAxisInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool USBAxisInput::tick_digital()
{
    return m_device->tick_analog(m_input.axis) != 0;
}
uint16_t USBAxisInput::tick_analog()
{
    return m_device->tick_analog(m_input.axis);
}
void USBAxisInput::setup()
{
}
USBButtonInput::USBButtonInput(proto_USBButtonInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool USBButtonInput::tick_digital()
{
    return m_device->tick_digital(m_input.button);
}
uint16_t USBButtonInput::tick_analog()
{
    return m_device->tick_digital(m_input.button) ? UINT16_MAX : 0;
}
void USBButtonInput::setup()
{
}
KeyboardKeyInput::KeyboardKeyInput(proto_KeyboardKeyInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool KeyboardKeyInput::tick_digital()
{
    return 0;
}
uint16_t KeyboardKeyInput::tick_analog()
{
    return 0;
}
void KeyboardKeyInput::setup()
{
}
MouseButtonInput::MouseButtonInput(proto_MouseButtonInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool MouseButtonInput::tick_digital()
{
    return 0;
}
uint16_t MouseButtonInput::tick_analog()
{
    return 0;
}
void MouseButtonInput::setup()
{
}
MouseAxisInput::MouseAxisInput(proto_MouseAxisInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool MouseAxisInput::tick_digital()
{
    return 0;
}
uint16_t MouseAxisInput::tick_analog()
{
    return 0;
}
void MouseAxisInput::setup()
{
}