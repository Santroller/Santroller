#pragma once
#include <stdint.h>
#include "config.pb.h"
#include <stdio.h>
#include "base.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/midi/midi.h"
#include <memory>
#include <vector>

#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
#define USB_PACKET_SIZE 4
typedef struct
{
    uint pos;
    uint actual_size;
    uint8_t data[32];
    bool sysex_in_progress;
} cable_state_t;
class MidiDeviceWithChannel;
class MidiDevice : public Device
{
    friend class MidiHost;

public:
    MidiDevice(uint16_t id, bool usbBased);
    virtual ~MidiDevice();
    void processMidiData(uint8_t *data, uint16_t len);
    virtual void update(bool full_poll, bool send_events);
    virtual void rescan(bool first);
    uint16_t readMidiNote(uint8_t channel, uint8_t note);
    uint16_t readMidiControlChange(uint8_t channel, uint8_t cc);
    int16_t readMidiPitchBend(uint8_t channel);
    std::shared_ptr<MidiDeviceWithChannel> getDeviceForChannel(uint8_t channel);

private:
    // Endpoint stream
    struct
    {
        tu_edpt_stream_t tx;
        tu_edpt_stream_t rx;

        uint8_t rx_ff_buf[TUH_EPSIZE_BULK_MAX];
        uint8_t tx_ff_buf[TUH_EPSIZE_BULK_MAX];
    } ep_stream;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[TUH_EPSIZE_BULK_MAX];
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[TUH_EPSIZE_BULK_MAX];
    uint8_t midiVelocities[16][128];
    int16_t midiPitchWheel[16];
    uint8_t midiControlChanges[16][128];
    bool drumMode;
    bool usbBased;
    cable_state_t cable_status[16];
    uint8_t usb_pos = 0;
    std::vector<std::shared_ptr<MidiDeviceWithChannel>> channelDevices;
};

class MidiDeviceWithChannel : public Device
{
public:
    MidiDeviceWithChannel(uint16_t id, uint8_t channel, std::shared_ptr<MidiDevice> midi_device) : Device(id), m_channel(channel), m_midi_device(midi_device) {}
    ~MidiDeviceWithChannel() {}
    uint16_t readMidiNote(uint8_t note);
    uint16_t readMidiControlChange(uint8_t cc);
    int16_t readMidiPitchBend();
    void update(bool full_poll, bool send_events) {};
    bool is_wii_extension(WiiExtType type) { return false; }
    bool is_usb_device(proto_SpecificUsbDevice type) { return false; }
    bool is_usb_type(SubType type) { return false; }
    bool is_bluetooth_device(proto_SpecificUsbDevice type) { return false; }
    bool is_bluetooth_type(SubType type) { return false; }
    bool is_ps2_device(PS2ControllerType type) { return false; }
    bool has_midi_channel(uint8_t channel) { return m_channel == channel; }
    bool using_pin(uint8_t pin) { return m_midi_device->using_pin(pin); }

private:
    uint8_t m_channel;
    std::shared_ptr<MidiDevice> m_midi_device;
};