#pragma once
#include "emulation/usb/hid_device.h"
#include "protocols/keyboardmania.hpp"

// In ModeHid this keeps our VID / PID so the tool can still find it, ModeKeyboardMania is
// the real controller as the PS2 expects it, with nothing else on the bus.
class KeyboardManiaDevice : public HIDDevice {
public:
    explicit KeyboardManiaDevice(bool native) : m_native(native) {}
    void initialize() override;
    void process(bool full_poll, bool send_events) override;
    size_t compatible_section_descriptor(uint8_t *, size_t) override { return 0; }
    size_t config_descriptor(uint8_t *dest, size_t remaining) override;
    size_t device_name(uint8_t idx, char *desc) override;
    void device_descriptor(tusb_desc_device_t *desc) override;
    const uint8_t *report_descriptor() override;
    uint16_t report_desc_len() override;
    uint16_t get_report(uint8_t id, hid_report_type_t type, uint8_t *buffer, uint16_t len) override;
    void set_report(uint8_t id, hid_report_type_t type, uint8_t const *buffer, uint16_t len) override;
private:
    bool m_native;
    uint8_t m_manufacturer_strid = 0;
    KeyboardManiaInputReport m_input = {KEYBOARDMANIA_HEADER, 0, 0, {}};
};
