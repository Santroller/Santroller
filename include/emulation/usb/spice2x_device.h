#pragma once
#include "emulation/usb/hid_device.h"
#include "protocols/spice2x.hpp"

class Spice2xDevice : public HIDDevice {
public:
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
    Spice2xInputReport m_input = {SPICE2X_INPUT_ID, 8, 0};
    uint8_t m_led_strid = 0;
    uint8_t m_descriptor[384] = {};
};
