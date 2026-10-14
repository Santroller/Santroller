#pragma once
#include "base.hpp"
#include "midi.hpp"
#include "device.pb.h"
#include "input_enums.pb.h"
#include "wii_extension.hpp"
class WiiDevice : public MidiDevice
{
public:
    WiiDevice(const DeviceReloadState* state, proto_WiiDevice device, uint16_t id);
    ~WiiDevice();
    void begin();
    void end(bool full);
    void rescan(bool first);
    void update(bool full_poll, bool send_events);
    uint16_t read_axis(proto_WiiAxisType type);
    bool read_button(proto_WiiButtonType type);
    uint16_t read_button_pressure(proto_WiiButtonType type);
    bool is_wii_extension(WiiExtType type);
    bool is_wii_device() const override { return true; }
    // A drum kit acts as a MIDI device on channel 10, so a MIDI channel 10 assignment can claim it
    bool has_midi_channel(uint8_t channel) override
    {
        return channel == WII_DRUM_MIDI_CHANNEL && m_extension.mType == WiiExtType::WiiGuitarHeroDrums;
    }
    bool using_pin(uint8_t pin);
    void save_reload_state(DeviceReloadState& state) const override;
    bool matches_reload_config(const proto_Device &config) const override;
    void set_euphoria_led(bool state) override;
    bool has_euphoria_led() const override;

protected:
    bool mark_channel_seen(uint8_t channel) override;

private:
    WiiExtension m_extension;
    proto_WiiDevice m_device;
    uint32_t m_last_value = 0;
    WiiExtType m_lastExtType = WiiExtType::WiiNoExtension;
    bool m_has_scanned = false;
    MidiStaticBuffers<32, 0, 32, 1> m_midi_buffers;
};