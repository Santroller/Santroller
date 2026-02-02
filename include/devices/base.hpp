#pragma once
#include <stdint.h>
#include "config.pb.h"
class Device
{
public:
    Device(uint16_t id) : m_id(id) {}
    virtual ~Device() {}
    virtual void update(bool full_poll) = 0;
    virtual void load_devices() = 0;
    virtual bool is_wii_extension(WiiExtType type);
    virtual bool is_usb_device(proto_SpecificUsbDevice type);
    virtual bool is_usb_type(SubType type);
    virtual bool is_bluetooth_device(proto_SpecificUsbDevice type);
    virtual bool is_bluetooth_type(SubType type);
    virtual bool is_ps2_device(PS2ControllerType type);
    uint16_t m_id;

protected:
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