#pragma once
#include <stdint.h>
class Device
{
public:
    Device(uint16_t id): m_id(id) {}
    virtual ~Device() {}
    virtual void update(bool resend_events)=0;
protected:
    uint16_t m_id;
    bool m_lastConnected;
};