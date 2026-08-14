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
class MidiDevice : public Device
{
    friend class MidiHost;

public:
    MidiDevice(uint16_t id, bool usbBased);
    virtual ~MidiDevice();
    void processMidiData(uint8_t *data, uint16_t len);
    virtual void update(bool full_poll, bool send_events);
    void rescan(bool first);
    uint16_t readMidiNote(uint8_t channel, uint8_t note);
    uint16_t readMidiControlChange(uint8_t channel, uint8_t cc);
    int16_t readMidiPitchBend(uint8_t channel);
    bool readProGuitarButton(proto_ProGuitarButtonType button);
    uint16_t readProGuitarAxis(proto_ProGuitarAxisType axis);
    bool has_midi_channel(uint8_t channel) { return seenChannels[channel]; }

private:
    // Endpoint stream
    struct
    {
        tu_edpt_stream_t tx;
        tu_edpt_stream_t rx;

        uint8_t rx_ff_buf[512];
        uint8_t tx_ff_buf[512];
    } ep_stream;
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_in_buf[TUH_EPSIZE_BULK_MAX];
    CFG_TUSB_MEM_ALIGN uint8_t m_ep_out_buf[TUH_EPSIZE_BULK_MAX];
    uint8_t midiVelocities[16][128];
    int16_t midiPitchWheel[16];
    uint8_t midiControlChanges[16][128];
    uint8_t midiFrets[6];
    uint8_t midiStringVelocities[6];
    bool seenChannels[18];
    ProGuitar_Sysex_Buttons_t midiButtons;
    bool drumMode;
    bool usbBased;
    cable_state_t cable_status[16];
    uint8_t usb_pos = 0;
};

class ProGuitarMidiDevice : public Device
{
public:
    ProGuitarMidiDevice(uint16_t id, std::shared_ptr<MidiDevice> midi_device) : Device(id), m_midi_device(midi_device) {}
    ~ProGuitarMidiDevice() {}
    bool readProGuitarButton(proto_ProGuitarButtonType button);
    uint16_t readProGuitarAxis(proto_ProGuitarAxisType axis);
    void update(bool full_poll, bool send_events) {};
    void begin() {};
    void end(bool full) {};
    bool is_wii_extension(WiiExtType type) { return false; }
    bool is_usb_device(proto_SpecificUsbDevice type) { return false; }
    bool is_usb_type(SubType type) { return false; }
    bool is_bluetooth_device(proto_SpecificUsbDevice type) { return false; }
    bool is_bluetooth_type(SubType type) { return false; }
    bool is_ps2_device(PS2ControllerType type) { return false; }
    bool has_midi_channel(uint8_t channel) { return MIDI_CHANNEL_PROGUITAR_MUSTANG == channel || MIDI_CHANNEL_PROGUITAR_SQUIER == channel; }
    bool using_pin(uint8_t pin) { return m_midi_device->using_pin(pin); }

private:
    std::shared_ptr<MidiDevice> m_midi_device;
};