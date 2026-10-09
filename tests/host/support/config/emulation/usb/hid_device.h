#pragma once
// Fake emulation/usb/hid_device.h: the config interface is a singleton the loader always keeps
#include <memory>
#include "emulation/usb/device.hpp"

class HIDConfigDevice : public UsbDevice
{
public:
    static inline std::shared_ptr<HIDConfigDevice> instance = std::make_shared<HIDConfigDevice>();
};
