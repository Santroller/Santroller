#pragma once
#include <stdint.h>
#include "config.pb.h"
#include <stdio.h>
#include "base.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/midi/midi.h"
#include "protocols/controller_reports.hpp"
#include <memory>
#include <vector>

#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
#define MIDI_CHANNEL_PROGUITAR_SQUIER 16
#define MIDI_CHANNEL_PROGUITAR_MUSTANG 17
#define MIDI_SYSEX_ID_PROGUITAR_SQUIER 0x08
#define MIDI_SYSEX_ID_PROGUITAR_MUSTANG 0x0A
#define USB_PACKET_SIZE 4
typedef struct
{
    uint status;
    uint pos;
    uint actual_size;
    uint8_t data[32];
    bool sysex_in_progress;
} cable_state_t;

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
    uint8_t read_midi_note(uint8_t channel, uint8_t note) const;
    bool is_midi_note_pressed(uint8_t channel, uint8_t note) const;
    uint16_t read_midi_control_change(uint8_t channel, uint8_t cc);
    int16_t read_midi_pitch_bend(uint8_t channel);
    bool read_pro_guitar_button(proto_ProGuitarMidiButtonType button);
    uint16_t read_pro_guitar_axis(proto_ProGuitarAxisType axis);
    bool has_midi_channel(uint8_t channel) { return seenChannels[channel]; }
    bool is_assignable() const override { return true; }
    void save_reload_state(DeviceReloadState& state) const override;

private:
    // Endpoint stream
    struct
    {
        tu_edpt_stream_t tx;
        tu_edpt_stream_t rx;
    } ep_stream;

    static constexpr size_t MIDI_NOTE_EVENT_CAPACITY = 32;
    struct MidiNoteEvent
    {
        uint16_t sequence;
        uint8_t channel;
        uint8_t note;
        uint8_t velocity;
    };
    MidiNoteEvent midiNoteEvents[MIDI_NOTE_EVENT_CAPACITY];
    uint8_t midiNoteEventHead = 0;
    uint8_t midiNoteEventCount = 0;
    uint16_t midiNoteEventSequence = 0;
    int16_t midiPitchWheel[16];
    uint8_t *midiControlChanges[16] = {};
    uint8_t *midiNoteVelocity[16] = {};
    uint8_t midiFrets[6];
    uint8_t midiStringVelocities[6];
    bool seenChannels[18];
    ProGuitar_Sysex_Buttons_t midiButtons;
    bool usbBased;
    cable_state_t *cable_status = nullptr;
    uint8_t m_max_cables = 1;
    uint8_t usb_pos = 0;

    void push_midi_note_event(uint8_t channel, uint8_t note, uint8_t velocity);
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
    bool using_pin(uint8_t pin) { return m_midi_device->using_pin(pin); }

private:
    std::shared_ptr<MidiDevice> m_midi_device;
};