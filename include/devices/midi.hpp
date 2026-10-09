#pragma once
#include <stdint.h>
#include "config.pb.h"
#include <stdio.h>
#include "base.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/midi/midi.h"
#include "protocols/controller_reports.hpp"
#include "protocols/midi_input.hpp"
#include <memory>
#include <vector>

struct MidiBufferConfig
{
    uint8_t *rx_ff_buf = nullptr;
    uint16_t rx_ff_bufsize = 0;
    uint8_t *m_ep_in_buf = nullptr;
    uint8_t *tx_ff_buf = nullptr;
    uint16_t tx_ff_bufsize = 0;
    uint8_t *m_ep_out_buf = nullptr;
    uint8_t max_cables = 1;
};

template <uint16_t RX_SIZE = 64, uint16_t TX_SIZE = 0, uint16_t EP_SIZE = 64, uint8_t MAX_CABLES = 1>
struct MidiStaticBuffers
{
    uint8_t rx_ff_buf[RX_SIZE > 0 ? RX_SIZE : 1];
    CFG_TUSB_MEM_ALIGN uint8_t ep_in_buf[EP_SIZE > 0 ? EP_SIZE : 1];
    uint8_t tx_ff_buf[TX_SIZE > 0 ? TX_SIZE : 1];
    CFG_TUSB_MEM_ALIGN uint8_t ep_out_buf[TX_SIZE > 0 ? EP_SIZE : 1];

    MidiBufferConfig config()
    {
        MidiBufferConfig cfg;
        cfg.rx_ff_buf = RX_SIZE > 0 ? rx_ff_buf : nullptr;
        cfg.rx_ff_bufsize = RX_SIZE;
        cfg.m_ep_in_buf = RX_SIZE > 0 ? ep_in_buf : nullptr;
        cfg.tx_ff_buf = TX_SIZE > 0 ? tx_ff_buf : nullptr;
        cfg.tx_ff_bufsize = TX_SIZE;
        cfg.m_ep_out_buf = TX_SIZE > 0 ? ep_out_buf : nullptr;
        cfg.max_cables = MAX_CABLES;
        return cfg;
    }
};

class MidiDevice : public Device
{
    friend class MidiHost;

public:
    MidiDevice(const DeviceReloadState* state, uint16_t id, bool usbBased, const MidiBufferConfig &buffer_config);
    virtual ~MidiDevice();
    void init_buffers(const MidiBufferConfig &buffer_config);
    void process_midi_data(uint8_t *data, uint16_t len);
    virtual void update(bool full_poll, bool send_events);
    void rescan(bool first);
    bool consume_midi_note_event(uint8_t channel, uint8_t note, uint16_t &sequence, uint16_t &velocity);
    bool peek_midi_note_event(uint8_t channel, uint8_t note, uint16_t &sequence, uint16_t &velocity) const;
    uint8_t read_midi_note(uint8_t channel, uint8_t note) const;
    bool is_midi_note_pressed(uint8_t channel, uint8_t note) const;
    uint16_t read_midi_control_change(uint8_t channel, uint8_t cc);
    // 14 bit, 0 - 16383 with MIDI_PITCH_BEND_CENTER at rest
    uint16_t read_midi_pitch_bend(uint8_t channel);
    bool read_pro_guitar_button(proto_ProGuitarMidiButtonType button);
    uint16_t read_pro_guitar_axis(proto_ProGuitarAxisType axis);
    bool has_midi_channel(uint8_t channel) { return m_midi.has_midi_channel(channel); }
    bool has_any_midi_channel() const
    {
        for (int i = 0; i < 18; i++)
        {
            if (m_midi.has_midi_channel(i)) return true;
        }
        return false;
    }
    bool is_assignable() const override { return true; }
    bool is_midi_device() const override { return true; }
    void save_reload_state(DeviceReloadState& state) const override;

protected:
    virtual bool mark_channel_seen(uint8_t channel);
    // Whether data arrives as 4 byte USB MIDI packets rather than a plain MIDI byte stream
    void set_usb_packets(bool usb_packets) { m_midi.set_usb_packets(usb_packets); }

private:
    // Endpoint stream
    struct
    {
        tu_edpt_stream_t tx;
        tu_edpt_stream_t rx;
    } ep_stream;

    // Parser and state for everything received
    MidiInput m_midi;
    bool usbBased;
};

class ProGuitarMidiDevice : public Device
{
public:
    ProGuitarMidiDevice(uint16_t id, std::shared_ptr<MidiDevice> midi_device) : Device(id), m_midi_device(midi_device) {}
    ~ProGuitarMidiDevice() {}
    bool read_pro_guitar_button(proto_ProGuitarMidiButtonType button);
    uint16_t read_pro_guitar_axis(proto_ProGuitarAxisType axis);
    void update(bool full_poll, bool send_events) {};
    void begin() {};
    void end(bool full) {};
    bool is_wii_extension(WiiExtType type) { return false; }
    bool is_usb_device(proto_SpecificUsbDevice type) { return false; }
    bool is_usb_type(SubType type) { return false; }
    bool is_bluetooth_device(proto_SpecificBluetoothDevice type) { return false; }
    bool is_bluetooth_type(SubType type) { return false; }
    bool is_ps2_device(PS2ControllerType type) { return false; }
    bool has_midi_channel(uint8_t channel) { return MIDI_CHANNEL_PROGUITAR_MUSTANG == channel || MIDI_CHANNEL_PROGUITAR_SQUIER == channel; }
    bool is_pro_guitar_midi_device() const override { return true; }
    bool using_pin(uint8_t pin) { return m_midi_device->using_pin(pin); }

private:
    std::shared_ptr<MidiDevice> m_midi_device;
};