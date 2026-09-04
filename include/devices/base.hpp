#pragma once
#include <stdint.h>
#include "config.pb.h"
#include "commands.pb.h"
#include <stdio.h>

struct DeviceReloadState
{
    bool valid = false;
    bool seen_midi_channels[18] = {};
    uint32_t last_value = 0;
    uint32_t debounce_time = 0;
    bool toggle_value = false;
    WiiExtType wii_extension = WiiExtType::WiiNoExtension;
    PS2ControllerType ps2_controller = PS2ControllerType::PS2ControllerTypeUnknown;
};

class Device
{
public:
    Device(uint16_t id) : m_id(id) {}
    virtual ~Device() {  }
    virtual void begin() = 0;
    virtual void end(bool full) = 0;
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual bool is_wii_extension(WiiExtType type);
    virtual bool is_usb_device(proto_SpecificUsbDevice type);
    virtual bool is_usb_type(SubType type);
    virtual bool is_bluetooth_device(proto_SpecificUsbDevice type);
    virtual bool is_bluetooth_type(SubType type);
    virtual bool is_ps2_device(PS2ControllerType type);
    virtual bool has_midi_channel(uint8_t channel);
    virtual bool using_pin(uint8_t pin) = 0;
    virtual uint32_t source_id() const { return m_id; }
    virtual void rescan(bool first);
    virtual void handle_command(proto_Command command) {};
    virtual void save_reload_state(DeviceReloadState& state) const { state.valid = true; }
    uint16_t m_id;
    bool still_connected = false;

protected:
    bool m_lastConnected;
};

class LedDevice : public Device
{
public:
    LedDevice(uint16_t id, uint16_t led_count, bool supportsColour, bool supports_brightness);
    virtual ~LedDevice() {}
    virtual void update(bool full_poll, bool send_events) = 0;
    void set_led(uint8_t idx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
    bool supportsColour() { return m_supportsColour; }
    bool supports_brightness() { return m_supportsBrightness; }

protected:
    uint32_t* led_state;
    uint32_t* prev_led_state;
    bool m_supportsColour;
    bool m_supportsBrightness;
};