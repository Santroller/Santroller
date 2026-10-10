#pragma once
#include "device.hpp"
#include "protocols/midi_output.hpp"

#define USB_MIDI_EP_SIZE 64

// A USB MIDI instrument: an Audio Control interface, then a MIDI Streaming interface with one cable
// each way. The profiles' MIDI mappings go out of the IN endpoint, and whatever the host sends is
// accepted and ignored.
class UsbMidiDevice : public UsbDevice
{
public:
    UsbMidiDevice();
    void initialize();
    void process(bool full_poll, bool send_events);
    uint8_t interface_count() const override { return 2; }
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);

private:
    void send_pending();
    uint8_t m_epin = 0;
    uint8_t m_epout = 0;
    uint8_t m_strid = 0;
    MidiState m_state;
    MidiOutput m_output;
    // what non MIDI mappings in the profile write their reports into
    uint8_t m_scratch_report[64];
    CFG_TUSB_MEM_ALIGN uint8_t m_epin_buf[USB_MIDI_EP_SIZE];
    CFG_TUSB_MEM_ALIGN uint8_t m_epout_buf[USB_MIDI_EP_SIZE];
};
