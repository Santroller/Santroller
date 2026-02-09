#pragma once
#include "input.hpp"
#include "input.pb.h"
#include "devices/usb.hpp"
#include <memory>
class USBAxisInput : public Input
{
public:
    USBAxisInput(proto_USBAxisInput input, std::shared_ptr<USBDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_USBAxisInput m_input;
    std::shared_ptr<USBDevice> m_device;
};
class USBButtonInput : public Input
{
public:
    USBButtonInput(proto_USBButtonInput input, std::shared_ptr<USBDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_USBButtonInput m_input;
    std::shared_ptr<USBDevice> m_device;
};
class KeyboardKeyInput : public Input
{
public:
    KeyboardKeyInput(proto_KeyboardKeyInput input, std::shared_ptr<USBDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_KeyboardKeyInput m_input;
    std::shared_ptr<USBDevice> m_device;
};
class MouseButtonInput : public Input
{
public:
    MouseButtonInput(proto_MouseButtonInput input, std::shared_ptr<USBDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_MouseButtonInput m_input;
    std::shared_ptr<USBDevice> m_device;
};
class MouseAxisInput : public Input
{
public:
    MouseAxisInput(proto_MouseAxisInput input, std::shared_ptr<USBDevice> device);
    bool tickDigital();
    uint16_t tickAnalog();

private:
    void setup();
    proto_MouseAxisInput m_input;
    std::shared_ptr<USBDevice> m_device;
};