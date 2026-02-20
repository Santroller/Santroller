#include "input/usb.hpp"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "stdio.h"

USBAxisInput::USBAxisInput(proto_USBAxisInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool USBAxisInput::tickDigital()
{
    return m_device->tick_analog(m_input.axis) != 0;
}
uint16_t USBAxisInput::tickAnalog()
{
    return m_device->tick_analog(m_input.axis);
}
void USBAxisInput::setup()
{
}
USBButtonInput::USBButtonInput(proto_USBButtonInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool USBButtonInput::tickDigital()
{
    return m_device->tick_digital(m_input.button);
}
uint16_t USBButtonInput::tickAnalog()
{
    return m_device->tick_digital(m_input.button) ? UINT16_MAX : 0;
}
void USBButtonInput::setup()
{
}
KeyboardKeyInput::KeyboardKeyInput(proto_KeyboardKeyInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool KeyboardKeyInput::tickDigital()
{
    return 0;
}
uint16_t KeyboardKeyInput::tickAnalog()
{
    return 0;
}
void KeyboardKeyInput::setup()
{
}
MouseButtonInput::MouseButtonInput(proto_MouseButtonInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool MouseButtonInput::tickDigital()
{
    return 0;
}
uint16_t MouseButtonInput::tickAnalog()
{
    return 0;
}
void MouseButtonInput::setup()
{
}
MouseAxisInput::MouseAxisInput(proto_MouseAxisInput input, std::shared_ptr<UsbHostInterface> device) : m_input(input), m_device(device)
{
}
bool MouseAxisInput::tickDigital()
{
    return 0;
}
uint16_t MouseAxisInput::tickAnalog()
{
    return 0;
}
void MouseAxisInput::setup()
{
}