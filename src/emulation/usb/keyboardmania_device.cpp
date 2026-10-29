#include "utils.h"
#include "emulation/usb/keyboardmania_device.h"
#include "main.hpp"
#include "managers/config_manager.hpp"
#include "managers/profile_manager.hpp"
#include "tusb.h"
#include <cassert>
#include <cstring>

void KeyboardManiaDevice::initialize() {
    m_epin = next_epin();
    m_strid = next_strid();
    m_manufacturer_strid = next_strid();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
}

void KeyboardManiaDevice::process(bool full_poll, bool send_events) {
    // A PC reads the HID descriptor, the PS2 never does as it has no driver for our VID / PID,
    // so swap to the real controller for it. This has to run before the tud_ready() check, as
    // the PS2 never configures the device either.
    auto &detection = UsbDetectionState::instance();
    if (!m_native && !mode_recently_changed() && detection.should_infer_console(millis()) &&
        !detection.seen_hid_descriptor_read()) {
        ConfigManager::instance().host_detected(ModeKeyboardMania);
    }
    if (!tud_ready()) {
        for (auto const &profile : profiles)
            for (auto const &led : profile->leds) led->update(full_poll, send_events);
        return;
    }
    if (!ready()) return;
    m_input.buttons = 0;
    for (auto const &profile : profiles) {
        for (auto const &mapping : profile->mappings) {
            mapping->update(full_poll, send_events);
            mapping->update_keyboardmania(reinterpret_cast<uint8_t *>(&m_input));
        }
        for (auto const &led : profile->leds) led->update(full_poll, send_events);
    }
    send_report(sizeof(m_input), 0, &m_input);
}

size_t KeyboardManiaDevice::config_descriptor(uint8_t *dest, size_t remaining) {
    uint8_t descriptor[] = {TUD_HID_DESCRIPTOR(interface_id, m_strid,
        HID_ITF_PROTOCOL_NONE, sizeof(keyboardmania_report_descriptor), m_epin, sizeof(m_input), 1)};
    // Byte for byte the real controller: its interface reuses the (empty) product string, it is HID
    // 1.10 and it polls every 4ms
    if (m_native) {
        descriptor[8] = STRID_PRODUCT;
        descriptor[11] = 0x10;
        descriptor[sizeof(descriptor) - 1] = 4;
    }
    assert(sizeof(descriptor) <= remaining);
    memcpy(dest, descriptor, sizeof(descriptor));
    return sizeof(descriptor);
}

size_t KeyboardManiaDevice::device_name(uint8_t idx, char *desc) {
    static constexpr char name[] = "USB Multipurpose Controller";
    static constexpr char manufacturer[] = "KONAMI";
    if (idx == m_strid) {
        memcpy(desc, name, sizeof(name));
        return sizeof(name);
    }
    if (idx == m_manufacturer_strid) {
        memcpy(desc, manufacturer, sizeof(manufacturer));
        return sizeof(manufacturer);
    }
    return 0;
}

void KeyboardManiaDevice::device_descriptor(tusb_desc_device_t *desc) {
    if (!m_native) return;
    desc->bcdUSB = 0x0110;
    desc->idVendor = KEYBOARDMANIA_VID;
    desc->idProduct = KEYBOARDMANIA_PID;
    // The game's driver never configures the device unless these are 1 and 2, like the real controller,
    // even though it never reads the strings
    desc->iManufacturer = STRID_MANUFACTURER;
    desc->iProduct = STRID_PRODUCT;
    desc->iSerialNumber = 0;
}

const uint8_t *KeyboardManiaDevice::report_descriptor() {
    UsbDetectionState::instance().mark_hid_descriptor_read();
    return keyboardmania_report_descriptor;
}
uint16_t KeyboardManiaDevice::report_desc_len() { return sizeof(keyboardmania_report_descriptor); }

uint16_t KeyboardManiaDevice::get_report(uint8_t id, hid_report_type_t type, uint8_t *buffer, uint16_t len) {
    if (type != HID_REPORT_TYPE_INPUT || id || len < sizeof(m_input))
        return 0;
    memcpy(buffer, &m_input, sizeof(m_input));
    return sizeof(m_input);
}

void KeyboardManiaDevice::set_report(uint8_t, hid_report_type_t, uint8_t const *, uint16_t) {
}
