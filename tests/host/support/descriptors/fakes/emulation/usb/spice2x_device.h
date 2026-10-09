#pragma once
// Fake Spice2xDevice declaration, so the real spice2x_device.cpp (and the descriptor it builds in
// initialize()) can be compiled without the USB device stack. Members are public for the tests.
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include "tusb.h"
#include "protocols/spice2x.hpp"

struct FakeSpiceMapping
{
    void update(bool, bool) {}
    void update_xinput(uint8_t *) {}
};
struct FakeSpiceLed
{
    void update(bool, bool) {}
};
struct FakeSpiceProfile
{
    std::vector<std::shared_ptr<FakeSpiceMapping>> mappings;
    std::vector<std::shared_ptr<FakeSpiceLed>> leds;
};

class Spice2xDevice
{
public:
    void initialize();
    void process(bool full_poll, bool send_events);
    size_t config_descriptor(uint8_t *dest, size_t remaining);
    size_t device_name(uint8_t idx, char *desc);
    void device_descriptor(tusb_desc_device_t *desc);
    const uint8_t *report_descriptor();
    uint16_t report_desc_len();
    uint16_t get_report(uint8_t id, hid_report_type_t type, uint8_t *buffer, uint16_t len);
    void set_report(uint8_t id, hid_report_type_t type, uint8_t const *buffer, uint16_t len);

    // what UsbDevice / HIDDevice provide
    static inline uint8_t s_strid = 5;
    static uint8_t next_epin() { return 0x81; }
    static uint8_t next_epout() { return 0x01; }
    static uint8_t next_strid() { return s_strid++; }
    void process_suspended(bool, bool) {}
    bool ready() { return false; }
    bool send_report(uint8_t, uint8_t, void const *) { return false; }
    uint8_t interface_id = 0;
    std::vector<std::shared_ptr<FakeSpiceProfile>> profiles;
    uint8_t m_epin = 0;
    uint8_t m_epout = 0;
    uint8_t m_strid = 0;

    Spice2xInputReport m_input = {SPICE2X_INPUT_ID, 8, 0};
    uint8_t m_led_strid = 0;
    uint8_t m_descriptor[384] = {};
};
