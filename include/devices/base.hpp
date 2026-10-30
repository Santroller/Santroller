#pragma once
#include <stdint.h>
#include <optional>
#include <memory>
#include "devices/hardware_id.hpp"
#include "config.pb.h"
#include "commands.pb.h"
#include <stdio.h>

typedef enum
{
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
    // Whether a cycling device's forward / backward inputs were down
    bool cycle_forward_held = false;
    bool cycle_backward_held = false;
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
    uint8_t ps2_rumble_large = 0;
    uint8_t ps2_rumble_small = 0;
    uint8_t ps2_data[32] = {};
    uint8_t ps2_lastInputs[32] = {};
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
    // Emulation State
    bool psx_emulation_communicating = false;
    bool wii_emulation_communicating = false;
    bool joybus_emulation_communicating = false;
    bool snes_emulation_communicating = false;
};

enum class DeviceSlotKind : uint8_t
{
    WiiExtension,
    PS2,
    USB,
    Bluetooth,
    MIDI,
    SNES,
    Joybus,
    None = 0xff
};

class Device
{
public:
    // The kind of slot a profile claims devices of this type through. Devices without one (None) are
    // always there once they're configured. Types with a slot hide these with their own.
    static constexpr DeviceSlotKind SLOT_KIND = DeviceSlotKind::None;
    // Whether device is of this type
    static bool is_instance(const Device &) { return true; }
    Device(uint16_t id) : m_id(id), m_serial(++s_next_serial) {}
    virtual ~Device() {}
    virtual bool valid() { return true; }
    virtual void begin() = 0;
    virtual void end(bool full) = 0;
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual bool is_wii_extension(WiiExtType type);
    virtual bool is_usb_device(proto_SpecificUsbDevice type);
    virtual bool is_usb_type(SubType type);
    virtual bool is_bluetooth_device(proto_SpecificBluetoothDevice type);
    virtual bool is_bluetooth_type(SubType type);
    virtual bool is_ps2_device(PS2ControllerType type);
    virtual bool is_snes_type(SNESControllerType type) { return false; }
    virtual bool is_joybus_type(JoybusControllerType type) { return false; }
    // An Xbox One controller that can authenticate with the console
    virtual bool is_xbox_one_pad() const { return false; }
    virtual bool is_wii_device() const { return false; }
    virtual bool is_midi_device() const { return false; }
    virtual bool is_usb_host_interface() const { return false; }
    virtual bool is_bluetooth_host_interface() const { return false; }
    virtual bool is_ps2_controller() const { return false; }
    virtual bool is_snes_device() const { return false; }
    virtual bool is_joybus_device() const { return false; }
    virtual bool has_midi_channel(uint8_t channel);
    virtual bool is_assignable() const { return false; }
    // For a device that runs others (a secondary pico), whether device_id is one of them
    virtual bool owns_sub_device(uint32_t device_id) const { (void)device_id; return false; }
    virtual bool using_pin(uint8_t pin) = 0;
    virtual uint32_t source_id() const { return m_id; }
    // Something that identifies this physical controller (a MAC address, or a serial it reports), so an
    // emulated pad driven by it can keep the same serial whichever slot or interface it ends up on
    virtual std::optional<uint64_t> hardware_id() const { return std::nullopt; }
    virtual void rescan(bool first);
    virtual void handle_command(proto_Command command) {};
    virtual void save_reload_state(DeviceReloadState &state) const { state.valid = true; }
    virtual bool matches_reload_config(const proto_Device &config) const { return false; }
    virtual void set_rumble(uint8_t left, uint8_t right) {}
    virtual void set_player_led(uint8_t player) {}
    virtual void set_lightbar(uint8_t r, uint8_t g, uint8_t b) {}
    virtual void set_euphoria_led(bool state) {}
    virtual void set_stagekit_led(uint8_t param, uint8_t command) {}
    virtual bool has_rumble() const { return false; }
    virtual bool has_player_led() const { return false; }
    virtual bool has_euphoria_led() const { return false; }
    virtual bool has_lightbar() const { return false; }
    virtual bool has_stagekit_led() const { return false; }
    uint16_t m_id;
    // Unique to this object, unlike m_id, which every interface on a USB host port shares and a
    // controller keeps when it is plugged in again
    uint32_t m_serial;
    bool still_connected = false;

protected:
    bool m_lastConnected;

private:
    static inline uint32_t s_next_serial = 0;
};

// The device as a T, or nullptr when it's missing or isn't one
template <typename T>
std::shared_ptr<T> device_cast(const std::shared_ptr<Device> &device)
{
    return device && T::is_instance(*device) ? std::static_pointer_cast<T>(device) : nullptr;
}

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
    uint32_t *led_state;
    uint32_t *prev_led_state;
    bool m_supportsColour;
    bool m_supportsBrightness;
};