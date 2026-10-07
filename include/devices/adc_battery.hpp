#pragma once
#include "base.hpp"
#include "device.pb.h"

// Estimates the battery level from its voltage on an ADC pin
class AdcBatteryDevice : public Device
{
public:
    ~AdcBatteryDevice() {}
    AdcBatteryDevice(proto_AdcBatteryDevice device, uint16_t id);
    void begin();
    void end(bool full);
    void update(bool full_poll, bool send_events);
    bool using_pin(uint8_t pin);

private:
    bool read_mv(uint32_t &mv);
    proto_AdcBatteryDevice m_device;
    bool m_valid;
    // smoothed, as the voltage dips whenever something draws more current
    uint32_t m_mv = 0;
    uint32_t m_last_read = 0;
    int32_t m_last_level = -1;
};
