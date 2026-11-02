#pragma once
#include "device.hpp"

// The most samples a packet carries, at 48kHz
#define USB_MIC_EP_SIZE (48 * 2)

// A USB Audio Class 1 microphone: an Audio Control interface, then an Audio Streaming interface whose
// alt setting 1 streams 16 bit mono over an isochronous IN endpoint. Plays whatever a USB microphone on
// the host port picks up (see MicStream).
// On its own on a console it becomes a Logitech USB Microphone, as Wii games drive the microphone
// themselves and only look for that one.
class UsbMicDevice : public UsbDevice
{
public:
    UsbMicDevice();
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
    // Whether no other profile was on USB in HID mode, before a console was found. A 360 or an Xbox One
    // only takes the microphone on its own, as the Logitech one
    static bool alone_in_hid() { return s_alone_in_hid; }

private:
    static bool s_alone_in_hid;
    bool interface_request(uint8_t stage, tusb_control_request_t const *request);
    bool endpoint_request(uint8_t stage, tusb_control_request_t const *request);
    // No other profile is on USB
    bool alone() const;
    // Alone and on a console, so the whole device is the Logitech microphone
    bool standalone() const;
    void set_streaming(bool streaming);
    void set_volume(int16_t volume);
    void queue_packet();
    uint8_t m_epin = 0;
    uint8_t m_strid = 0;
    // The manufacturer, then the product after it, as a Logitech microphone
    uint8_t m_strid_logitech = 0;
    // Whatever the descriptor last told the host, 0x84 when standalone like the Logitech one
    uint8_t m_ep = 0;
    bool m_streaming = false;
    uint32_t m_rate = 48000;
    // Thousandths of a sample carried over to the next packet, for rates like 44.1kHz
    uint32_t m_remainder = 0;
    bool m_mute = false;
    // In 1/256 dB, as UAC1 has it
    int16_t m_volume = 0;
    // m_volume as a multiplier, 4096 is unity
    int32_t m_gain = 4096;
    CFG_TUSB_MEM_ALIGN uint8_t m_ctrl_buf[4];
    CFG_TUSB_MEM_ALIGN int16_t m_packet[USB_MIC_EP_SIZE / 2];
};
