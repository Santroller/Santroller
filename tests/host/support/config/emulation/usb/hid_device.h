#pragma once
// Fake emulation/usb/hid_device.h: the config interface is a singleton the loader always keeps
#include <memory>
#include "emulation/usb/device.hpp"
#include "config_fakes.hpp"

class HIDConfigDevice : public UsbDevice
{
public:
    static inline std::shared_ptr<HIDConfigDevice> instance = std::make_shared<HIDConfigDevice>();
    static bool tool_closed() { return fake_loader::state.tool_closed; }
    static bool tool_may_be_open() { return !fake_loader::state.tool_closed; }
};
