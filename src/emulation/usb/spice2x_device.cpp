#include "emulation/usb/spice2x_device.h"
#include "managers/profile_manager.hpp"
#include "protocols/xinput.hpp"
#include "tusb.h"
#include <cassert>
#include <cstring>

// Each LED has a distinct ordinal and a USB string index (patched at initialize).
#define SPICE_LED(ordinal) \
    0x05, 0x0a, 0x09, ordinal, 0xa1, 0x02, \
    0x05, 0x08, 0x09, 0x4b, 0x79, 0x00, \
    0x75, 0x08, 0x95, 0x01, 0x91, 0x02, 0xc0

static constexpr uint8_t report_desc[] = {
    0x05, 0x01, 0x09, 0x05, 0xa1, 0x01,
    0xa1, 0x02, 0x85, 0x01,
    0x09, 0x39, 0x75, 0x04, 0x95, 0x01,
    0x15, 0x00, 0x25, 0x07, 0x35, 0x00, 0x46, 0x3b, 0x01,
    0x65, 0x14, 0x81, 0x42,
    0x05, 0x09, 0x75, 0x01, 0x95, 0x0a,
    0x19, 0x01, 0x29, 0x0a, 0x15, 0x00, 0x25, 0x01,
    0x35, 0x00, 0x45, 0x01, 0x81, 0x02,
    0x75, 0x01, 0x95, 0x02, 0x81, 0x03,
    0x05, 0x09, 0x75, 0x01, 0x95, 0x0b,
    0x19, 0x0b, 0x29, 0x15, 0x15, 0x00, 0x25, 0x01,
    0x35, 0x00, 0x45, 0x01, 0x81, 0x02,
    0x75, 0x01, 0x95, 0x05, 0x81, 0x03, 0xc0,

    0xa1, 0x02, 0x85, 0x02,
    0x15, 0x00, 0x26, 0xff, 0x00, 0x35, 0x00, 0x46, 0xff, 0x00,
    SPICE_LED(0x01), SPICE_LED(0x02), SPICE_LED(0x03),
    SPICE_LED(0x04), SPICE_LED(0x05), SPICE_LED(0x06),
    SPICE_LED(0x07), SPICE_LED(0x08), SPICE_LED(0x09), 0xc0,

    0xa1, 0x02, 0x85, 0x03,
    0x15, 0x00, 0x26, 0xff, 0x00, 0x35, 0x00, 0x46, 0xff, 0x00,
    SPICE_LED(0x01), SPICE_LED(0x02), SPICE_LED(0x03), 0xc0, 0xc0
};
#undef SPICE_LED
static_assert(sizeof(report_desc) <= 384, "Spice2x descriptor exceeds storage");

void Spice2xDevice::initialize() {
    m_epin = next_epin();
    m_epout = next_epout();
    m_strid = next_strid();
    m_led_strid = next_strid();
    for (unsigned i = 1; i < 12; ++i) next_strid();
    memcpy(m_descriptor, report_desc, sizeof(report_desc));
    unsigned led = 0;
    for (size_t i = 0; i + 1 < sizeof(report_desc); ++i) {
        if (m_descriptor[i] == 0x79 && led < 12)
            m_descriptor[i + 1] = m_led_strid + led++;
    }
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);
}

void Spice2xDevice::process(bool full_poll, bool send_events) {
    if (tud_suspended()) {
        process_suspended(full_poll, send_events);
        return;
    }
    if (!tud_ready()) {
        for (auto const &profile : profiles)
            for (auto const &led : profile->leds) led->update(full_poll, send_events);
        return;
    }
    if (!ready()) return;
    XInputGamepad_Data_t gamepad = {};
    for (auto const &profile : profiles) {
        for (auto const &mapping : profile->mappings) {
            mapping->update(full_poll, send_events);
            mapping->update_xinput(reinterpret_cast<uint8_t *>(&gamepad));
        }
        for (auto const &led : profile->leds) led->update(full_poll, send_events);
    }
    uint8_t dpad = (gamepad.dpadUp ? 1 : 0) | (gamepad.dpadDown ? 2 : 0) |
                   (gamepad.dpadLeft ? 4 : 0) | (gamepad.dpadRight ? 8 : 0);
    static constexpr uint8_t hat[] = {8, 0, 4, 8, 6, 7, 5, 6, 2, 1, 3, 2, 8, 0, 4, 8};
    m_input.controller = hat[dpad] |
        (gamepad.a << 4) | (gamepad.b << 5) | (gamepad.x << 6) | (gamepad.y << 7) |
        (gamepad.rightShoulder << 8) | (gamepad.leftShoulder << 9) |
        (gamepad.start << 10) | (gamepad.back << 11) |
        (gamepad.guide << 12) | (gamepad.capture << 13);
    m_input.pad = (gamepad.start ? 1 : 0) | (gamepad.back ? 2 : 0) |
        (gamepad.x ? SpiceUpLeft : 0) | (gamepad.dpadUp ? SpiceUp : 0) |
        (gamepad.y ? SpiceUpRight : 0) | (gamepad.dpadLeft ? SpiceLeft : 0) |
        (gamepad.leftThumbClick ? SpiceCenter : 0) | (gamepad.dpadRight ? SpiceRight : 0) |
        (gamepad.a ? SpiceDownLeft : 0) | (gamepad.dpadDown ? SpiceDown : 0) |
        (gamepad.b ? SpiceDownRight : 0);
    send_report(sizeof(m_input), 0, &m_input);
}

size_t Spice2xDevice::config_descriptor(uint8_t *dest, size_t remaining) {
    uint8_t const descriptor[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, m_strid,
        HID_ITF_PROTOCOL_NONE, sizeof(report_desc), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(descriptor) <= remaining);
    memcpy(dest, descriptor, sizeof(descriptor));
    return sizeof(descriptor);
}

size_t Spice2xDevice::device_name(uint8_t idx, char *desc) {
    static constexpr char name[] = "Spice2x";
    static char const *const leds[] = {
        "Up Left", "Up", "Up Right", "Left", "Center", "Right",
        "Down Left", "Down", "Down Right", "Status Red", "Status Green", "Status Blue"
    };
    if (idx == m_strid) {
        memcpy(desc, name, sizeof(name));
        return sizeof(name);
    }
    if (idx >= m_led_strid && idx - m_led_strid < 12) {
        auto text = leds[idx - m_led_strid];
        size_t length = strlen(text) + 1;
        memcpy(desc, text, length);
        return length;
    }
    return 0;
}

void Spice2xDevice::device_descriptor(tusb_desc_device_t *desc) {
    desc->idVendor = SPICE2X_VID;
    desc->idProduct = SPICE2X_PID;
}

const uint8_t *Spice2xDevice::report_descriptor() { return m_descriptor; }
uint16_t Spice2xDevice::report_desc_len() { return sizeof(report_desc); }

uint16_t Spice2xDevice::get_report(uint8_t id, hid_report_type_t type, uint8_t *buffer, uint16_t len) {
    if (type != HID_REPORT_TYPE_INPUT || (id && id != SPICE2X_INPUT_ID) || len < sizeof(m_input))
        return 0;
    memcpy(buffer, &m_input, sizeof(m_input));
    return sizeof(m_input);
}

void Spice2xDevice::set_report(uint8_t, hid_report_type_t, uint8_t const *, uint16_t) {
    // LED output is not routed to panel/status LEDs yet.
}
