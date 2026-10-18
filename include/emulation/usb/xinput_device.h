#pragma once

#include "common/tusb_common.h"
#include "device/usbd.h"
#include "protocols/xinput.hpp"
#include "device.hpp"
#include "enums.pb.h"
#include "config/config.hpp"
#include "emulation/usb/usb_descriptors.h"

#ifndef CFG_TUD_XINPUT_EPSIZE
#define CFG_TUD_XINPUT_EPSIZE 64
#endif
// Every packet a receiver slot sends on its IN endpoint, except the 2 byte presence reply
#define XINPUT_WIRELESS_PACKET_SIZE 29
class XInputSecurityDevice : public UsbDevice
{
public:
    XInputSecurityDevice();
    void initialize();
    void process(bool full_poll, bool send_events);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
private:
    uint8_t m_strid;
};
class XInputGamepadDevice : public UsbDevice
{
public:
    XInputGamepadDevice();
    void initialize();
    void process(bool full_poll, bool send_events);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    uint8_t m_epin;
    uint8_t m_epout;
    uint8_t m_strid;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];

    static void reset();
    // The serial for the pad on this interface. It follows the controller claimed for the pad when that has an
    // id of its own, and is otherwise fixed to the board and interface, stable across reloads and reboots
    static uint32_t controller_serial(uint8_t itf);
    // What a genuine receiver answers its vendor id request (c0 01 wValue 1) with
    static void receiver_id(uint8_t id[7]);
    static uint8_t xinputInterfaces[4];
    static uint8_t lastIntfInput;
    static uint32_t last_caps_query_time;
    // serial reads at wIndex 0 rotate through the pads like the caps reads, on their own index
    static uint8_t lastIntfSerial;
    static uint32_t last_serial_query_time;
    static volatile uint32_t report_generation;
    // Cleared when the device stack starts, set once the host configures us
    static bool mounted_since_init;

private:
    // Receiver slot replies, sent on the IN endpoint ahead of the next input report
    bool send_wireless_reply();
    void handle_led(uint8_t led);
    void handle_rumble(uint8_t left, uint8_t right);
    void process_wireless_command(const uint8_t *buf, uint32_t len);
    bool m_wireless = false;
    bool m_send_presence = false;
    bool m_send_link = false;
    bool m_send_caps = false;
    XInputInputCapabilities_t input_capabilities() const;
    XInputGamepad_Data_t m_initial_report;
    XInputGamepad_Data_t m_last_report;
    bool m_has_last_report = false;
    uint32_t m_report_generation = 0;
    // the frame an unchanged report was skipped in, so the next isn't built until a new frame
    uint16_t m_unchanged_frame = 0;
    bool m_has_unchanged_frame = false;
    bool m_bus_suspended = false;
};