#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "wii_extension.hpp"
class WiiDevice : public Device
{
public:
    ~WiiDevice() {}
    WiiDevice(proto_WiiDevice device, uint16_t id);
    void update(bool resend_events);

private:
    proto_WiiDevice m_device;
    WiiExtension m_extension;
    uint32_t m_lastValue = 0;
    WiiExtType m_lastExtType = WiiExtType::WiiNoExtension;
};