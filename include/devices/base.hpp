#pragma once
#include <stdint.h>
#include "config.pb.h"
#include <stdio.h>
class Device
{
public:
    Device(uint16_t id) : m_id(id) {}
    virtual ~Device() { printf("~Device()\r\n"); }
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual bool is_wii_extension(WiiExtType type);
    virtual bool is_usb_device(proto_SpecificUsbDevice type);
    virtual bool is_usb_type(SubType type);
    virtual bool is_bluetooth_device(proto_SpecificUsbDevice type);
    virtual bool is_bluetooth_type(SubType type);
    virtual bool is_ps2_device(PS2ControllerType type);
    virtual bool using_pin(uint8_t pin) = 0;
    virtual void rescan(bool first);
    uint16_t m_id;

protected:
    bool m_lastConnected;
    bool resend = false;
};

class MidiDevice : public Device
{
public:
    MidiDevice(uint16_t id) : Device(id) {}
    virtual ~MidiDevice() { printf("~MidiDevice()\r\n"); }
    virtual uint16_t readMidiNote(uint8_t note) = 0;
    virtual uint16_t readMidiControlChange(uint8_t cc) = 0;
    virtual int16_t readMidiPitchBend() = 0;

protected:
    void processMidiData(uint8_t *data, uint16_t len);
};

class LedDevice : public Device
{
public:
    LedDevice(uint16_t id, bool supportsColour, bool supports_brightness);
    virtual ~LedDevice() {}
    virtual void update(bool full_poll, bool send_events) = 0;
    void set_led(uint8_t idx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    bool supportsColour() { return m_supportsColour; }
    bool supports_brightness() { return m_supportsBrightness; }

protected:
    uint32_t led_state[255];
    uint32_t prev_led_state[255];
    bool m_supportsColour;
    bool m_supportsBrightness;
};