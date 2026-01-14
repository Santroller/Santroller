#pragma once
#include <stdint.h>
class Device
{
public:
    Device(uint16_t id) : m_id(id) {}
    virtual ~Device() {}
    virtual void update(bool full_poll) = 0;

protected:
    uint16_t m_id;
    bool m_lastConnected;
    bool resend = false;
};

class LedDevice : public Device
{
public:
    LedDevice(uint16_t id, bool supportsColour);
    virtual ~LedDevice() {}
    virtual void update(bool full_poll) = 0;
    void set_led(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
    bool supportsColour() { return m_supportsColour; }

protected:
    uint32_t led_state[255];
    bool m_supportsColour;
};