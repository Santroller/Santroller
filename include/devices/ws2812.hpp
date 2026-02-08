#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "wii_extension.hpp"
#include "libws2812.hpp"
class WS2812Device : public LedDevice
{
public:
    ~WS2812Device() {}
    WS2812Device(proto_WS2812Device device, uint16_t id);
    void update(bool full_poll);
    bool using_pin(uint8_t pin);

private:
    WS2812 m_ws2812;
    proto_WS2812Device m_device;
};