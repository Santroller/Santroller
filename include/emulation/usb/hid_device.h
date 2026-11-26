
#pragma once
#include "emulation/keyboard_mouse.hpp"
#include "class/hid/hid.h"

#include "commands.pb.h"
#include "emulation/usb/usb_descriptors.h"
#include "device.hpp"
#include "events.pb.h"
#include "config/input_detector.hpp"

#include "tusb.h"

//--------------------------------------------------------------------+
// Class Driver Default Configure & Validation
//--------------------------------------------------------------------+

#if !defined(CFG_TUD_HID_EP_BUFSIZE) & defined(CFG_TUD_HID_BUFSIZE)
// TODO warn user to use new name later on
// #warning CFG_TUD_HID_BUFSIZE is renamed to CFG_TUD_HID_EP_BUFSIZE, please update to use the new name
#define CFG_TUD_HID_EP_BUFSIZE CFG_TUD_HID_BUFSIZE
#endif

#ifndef CFG_TUD_HID_EP_BUFSIZE
#define CFG_TUD_HID_EP_BUFSIZE 64
#endif

class HIDDevice : public UsbDevice
{
public:
    virtual ~HIDDevice() {};
    virtual const uint8_t *report_descriptor() = 0;
    virtual uint16_t report_desc_len() = 0;
    virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) = 0;
    virtual void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) = 0;
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    bool control_transfer(uint8_t stage, tusb_control_request_t const *request);
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len);
    bool send_report(uint8_t len, uint8_t report_id, void const *report);
    // Sends a report (without a report id) only if it changed since the last one sent, or the idle
    // interval passed, so the host still sees the current state if it missed one
    bool send_report_if_changed(uint8_t len, void const *report);
    // True once an unchanged report was skipped this frame. The host polls every frame, so building
    // another report before the next one would just run the mappings every loop instead of once per
    // poll, which time based logic like the cymbal glitch fix isn't written for. Also true until the
    // profiles' minimum report interval has passed since the last report.
    bool skip_report_this_frame();
    bool ready();

protected:
    const tusb_hid_descriptor_hid_t *hid_descriptor;
    uint8_t itf_protocol;  // Boot mouse or keyboard
    uint8_t protocol_mode; // Boot (0) or Report protocol (1)
    uint8_t idle_rate;     // up to application to handle idle rate
    bool clearedIn = false;
    bool clearedOut = false;
    uint8_t m_epin = 0;
    uint8_t m_epout = 0;
    // How much one OUT transfer waits for. A transfer only ends early on a short packet, so a device whose
    // OUT endpoint is smaller than this has to wait for exactly its reports
    uint16_t m_epout_xfer_len = CFG_TUD_HID_EP_BUFSIZE;
    uint8_t m_strid = 0;
    bool m_sent_first_report = false;
    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_HID_EP_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_HID_EP_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t ctrl_buf[CFG_TUD_HID_EP_BUFSIZE];
    uint8_t m_initial_report[CFG_TUD_HID_EP_BUFSIZE];
    uint8_t m_last_report[CFG_TUD_HID_EP_BUFSIZE];
    bool m_has_last_report = false;
    uint32_t m_last_report_us = 0;
    uint16_t m_unchanged_frame = 0;
    bool m_has_unchanged_frame = false;
};

// Where the configurator is talking to us from
enum class ConfigTransport
{
    Usb,
    Bluetooth,
    // Vendor control requests to the device, for WebUSB (Chrome on Android lists no HID devices, and WebUSB
    // can't claim a HID interface). Events wait for the tool to poll them, as there is no endpoint to send them on
    UsbControl,
};

// The vendor request (to the device) that carries config reports over UsbControl. wValue is the same as a
// HID GET_REPORT / SET_REPORT (report type << 8 | report id), and the buffers include the report id. Reading
// the ReportIdConfig input report takes the queued events. wIndex is fixed, so no other vendor request matches
#define SANTROLLER_CONFIG_VENDOR_REQUEST 0x53
#define SANTROLLER_CONFIG_VENDOR_INDEX 0x5343

class HIDConfigDevice : public HIDDevice
{
public:
    HIDConfigDevice();
    void initialize();
    void process(bool full_poll, bool send_events);
    // Always there for the config tool, a suspended host doesn't change that
    bool suspends_with_host() const override { return false; }
    static bool tool_closed();
    // Also true for a while after the device stack restarted under an open tool, as it reconnects
    static bool tool_may_be_open();
    static void reset_keepalive();
    // For a device stack restart: the tool, if open, will be back once the device re-enumerates
    static void restart_keepalive();
    // The config reloaded while the tool's keepalive had lapsed, so it gets told once it's back
    static void reload_missed();
    static void check_firmware_update_timeout();
    // From the main loop: once an app firmware update starts, end every device it doesn't need. Nothing
    // else gets to stall or crash the controller partway through, and it reboots into the update anyway
    static void tear_down_for_firmware_update();
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
    // Same as get_report / set_report (buffers include the report id), but for requests that arrived over another transport
    uint16_t get_report_from(ConfigTransport transport, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    // Returns false if the report was refused
    bool set_report_from(ConfigTransport transport, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
    // True if the config device is already processed as a USB instance
    bool is_registered();
    bool interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);
    // A SANTROLLER_CONFIG_VENDOR_REQUEST from the host. Returns false to stall it
    bool vendor_control_transfer(uint8_t stage, tusb_control_request_t const *request);
    static bool send_event(proto_Event event, bool now);
    // Non-blocking: true if an event can be queued without waiting on the host.
    static bool has_event_space();
    // Waits (servicing tud_task) until every queued event has been read by the tool, or the tool goes away
    static void flush_events(uint32_t timeout_us);
    static std::shared_ptr<HIDConfigDevice> instance;

private:
    void process_events();
    bool events_busy();
    // Encodes as many of the queued events as fit in max_size bytes (as many as possible, at least one if
    // it fits at all) into buffer. Returns how many it encoded, with the size in written
    pb_size_t encode_events(uint8_t *buffer, uint16_t max_size, size_t *written);
    // Drops the first count queued events, once they have been sent
    void drop_events(pb_size_t count);
    // The ReportIdConfig input report for a tool polling over UsbControl: the report id, then the events
    uint16_t take_events(uint8_t *buffer, uint16_t reqlen);
    void switch_transport(ConfigTransport transport);
    // A tool on the other transport is still open, so one on transport has to wait for it to close
    bool in_use_elsewhere(ConfigTransport transport) const;
    uint16_t handle_get_report_in_use(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void handle_command(proto_Command command);
    uint16_t handle_get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    bool handle_set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);
    void finish_firmware_update();
    ConfigTransport m_transport = ConfigTransport::Usb;
    // Handling a UsbControl request: the tool can't poll for events until it finishes, so nothing waits on them
    bool m_in_control_request = false;
    bool events_can_drain() const { return !(m_transport == ConfigTransport::UsbControl && m_in_control_request); }
    proto_EventList list;
    proto_PinDetectType m_detect_type;
    uint32_t detect_done = 0;
    uint32_t lastKeepAlive = 0;
    bool debug_enabled = false;
    bool cached = false;
    uint32_t start = 0;
    uint32_t selected_profile = 0;
    uint32_t selected_instance = 0;
    uint8_t current_pos = 1;
    uint64_t m_valid_pins;
    bool tool_seen = false;
    // Until this, the tool is still treated as open after the device stack restarted under it
    uint32_t tool_return_deadline = 0;
    bool just_loaded = false;
    bool profile_selected = false;
    // The keepalive lapsed while a profile was selected, so put it back when the tool returns
    bool tool_lapsed = false;
    bool tool_missed_reload = false;
    bool profile_changed = false;
    uint16_t last_adc_vals[NUM_ADC_CHANNELS];
    bool last_digital_vals[NUM_BANK0_GPIOS];
    // Digital pin detection: pins currently away from their idle level, when they last left it,
    // how often they've done so, and pins dropped for flicking too often
    uint64_t m_detect_changed;
    uint64_t m_detect_noisy;
    uint32_t m_detect_changed_at[NUM_BANK0_GPIOS];
    uint8_t m_detect_transitions[NUM_BANK0_GPIOS];
    InputDetector m_input_detector;
    // Pin pair scan (ScanPinPairsCommand): the pins being scanned, and for each driven pin the pins
    // it pulled low on the last scan and the ones last reported to the tool
    void start_pair_scan(uint64_t requested);
    void stop_pair_scan();
    void pair_scan_tick();
    uint64_t m_pair_pins = 0;
    uint32_t m_pair_last_scan = 0;
    uint64_t m_pair_last[NUM_BANK0_GPIOS];
    uint64_t m_pair_reported[NUM_BANK0_GPIOS];
    uint8_t fw_update_tmp[4096];
    proto_FirmwareUpdate update_state;
    std::shared_ptr<class SecondaryPicoDevice> m_target_secondary_pico = nullptr;
    // The upload is going to the radio firmware area rather than the app's download slot
    bool m_radio_update = false;
    // An app update started, so the devices get ended from the main loop (not from inside a USB callback)
    bool m_teardown_pending = false;
    // The devices were ended for an app update, so if it doesn't finish the config has to load again
    bool m_torn_down = false;
};
class HIDGamepadDevice : public HIDDevice
{
public:
    HIDGamepadDevice();
    uint16_t open(tusb_desc_interface_t const *itf_desc, uint16_t max_len) override;
    void initialize();
    void process(bool full_poll, bool send_events);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);

private:
    bool m_capabilities_pending = false;
    // Whether the descriptor has a battery report, and the last level sent in it
    bool m_battery = false;
    int16_t m_sent_battery = -1;
    int _FFB_effect_index;
    int _FFB_effect_duration;
    int _loop_count;
    int periodic_magnitude;
    int constant_force;
    int vibration_on;
};
class HIDKeyboardDevice : public HIDDevice
{
public:
    HIDKeyboardDevice();
    void initialize();
    void process(bool full_poll, bool send_events);
    size_t compatible_section_descriptor(uint8_t *desc, size_t remaining);
    size_t config_descriptor(uint8_t *desc, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
    void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);

private:
    KeyboardMouseReports m_reports;
};