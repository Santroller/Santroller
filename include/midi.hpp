#pragma once
#include <stdint.h>
#include <stdio.h>
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "class/midi/midi.h"
#include "midi_descriptors.h"

#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
#define MIDI_CHANNEL_PROGUITAR 16
#define USB_PACKET_SIZE 4

typedef struct
{
    uint pos;
    uint actual_size;
    uint8_t data[32];
    bool sysex_in_progress;
} cable_state_t;
class MidiDeviceWithChannel;
class MidiDevice 
{

public:
    MidiDevice(bool usbBased, Midi_Data_t* data);
    virtual ~MidiDevice();
    void processMidiData(uint8_t *data, uint16_t len);
    virtual void update(bool full_poll, bool send_events);
    void rescan(bool first);

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
    Midi_Data_t *midiData;
    bool usbBased;
    cable_state_t cable_status[16];
    uint8_t usb_pos = 0;
};
