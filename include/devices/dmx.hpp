#pragma once
#include "base.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "DmxOutput.h"
class DMXDevice : public LedDevice
{
public:
    ~DMXDevice() {}
    DMXDevice(proto_DMXDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);
    // Whether channel (1 based, as DMX numbers them) is one this sends
    bool has_channel(int32_t channel) const { return channel >= 1 && channel <= channel_count(); }


private:
    uint16_t channel_count() const { return m_device.channelCount < 0 ? 0 : (m_device.channelCount > 512 ? 512 : m_device.channelCount); }
    DmxOutput m_dmx;
    proto_DMXDevice m_device;
    // False when the pin is unset or the output couldn't get a state machine or DMA channel
    bool m_started = false;
};