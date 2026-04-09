#pragma once
#include <stdint.h>
#include "config.pb.h"
#include <stdio.h>
#include "base.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/midi/midi.h"


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

class MidiDevice : public Device
{
public:
    MidiDevice(uint16_t id, bool usbBased);
    virtual ~MidiDevice();
    uint16_t readMidiNote(uint8_t note);
    uint16_t readMidiControlChange(uint8_t cc);
    int16_t readMidiPitchBend();
    void processMidiData(uint8_t *data, uint16_t len);
    virtual void update(bool full_poll, bool send_events);

protected:
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
    uint8_t midiModWheel[16];
    uint8_t midiSustainPedal[16];
    bool drumMode;
    bool usbBased;
    cable_state_t cable_status[16];
    uint8_t usb_pos = 0;
};
