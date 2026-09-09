#pragma once
#include <stdint.h>
#include "config.pb.h"
#include "commands.pb.h"
#include <stdio.h>

typedef enum {
    WII_INIT_FINISH_ENC,
    WII_INIT_FB_0,
    WII_INIT_READ_ID_WRITE_PTR,
    WII_INIT_READ_ID_READ,
    WII_INIT_DRAWSOME,
    WII_INIT_CLASSIC_0,
    WII_INIT_CLASSIC_1,
    WII_INIT_CLASSIC_2,
    WII_INIT_CLASSIC_READ_ID_WRITE_PTR,
    WII_INIT_CLASSIC_READ_ID_READ,
    WII_INIT_READ_DATA_WRITE_PTR,
    WII_INIT_READ_DATA_READ,
    WII_INIT_ENABLE_ENC_0,
    WII_INIT_ENABLE_ENC_1,
    WII_INIT_ENABLE_ENC_2,
    WII_INIT_ENABLE_ENC_3,
    WII_INIT_ENC_READ_ID_WRITE_PTR,
    WII_INIT_ENC_READ_ID_READ,
    WII_INPUTS_WRITE_PTR,
    WII_INPUTS_READ,
    WII_INPUTS_UPDATE_LED
} wii_status_e;
struct DeviceReloadState
{
    bool valid = false;
    bool seen_midi_channels[18] = {};
    uint32_t last_value = 0;
    uint32_t debounce_time = 0;
    bool toggle_value = false;
    WiiExtType wii_extension = WiiExtType::WiiNoExtension;
    PS2ControllerType ps2_controller = PS2ControllerType::PS2ControllerTypeUnknown;
    // PS2 State
    PS2ControllerType ps2_type = PS2ControllerType::PS2ControllerTypeUnknown;
    bool ps2_valid = false;
    bool ps2_hasTapBar = false;
    bool ps2_missing = false;
    uint32_t ps2_last = 0;
    uint32_t ps2_lastInit = 0;
    uint32_t ps2_invalidCount = 0;
    uint8_t ps2_data[18] = {};
    uint8_t ps2_lastInputs[18] = {};
    uint8_t ps2_dataOut[18] = {};
    uint8_t ps2_idx = 0;
    uint8_t ps2_len = 0;
    uint8_t ps2_dataLen = 0;
    bool ps2_done = false;
    uint32_t ps2_packetDelay = 0;
    // Wii State
    bool wii_mFound = false;
    WiiExtType wii_mType = WiiExtType::WiiNoExtension;
    bool wii_hiRes = false;
    uint32_t wii_packetIssueCount = 0;
    uint8_t wii_mBufferIndex = 0;
    uint32_t wii_lastTick = 0;
    uint8_t wii_wiiBytes = 0;
    uint8_t wii_wiiPointer = 0;
    uint8_t wii_s_box = 0;
    uint8_t wii_m_block = 0;
    wii_status_e wii_status = WII_INIT_FINISH_ENC;
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
    virtual bool is_assignable() const { return false; }
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