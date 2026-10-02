#include "emulation/usb/pdloader_device.h"
#include "managers/profile_manager.hpp"
#include "protocols/xinput.hpp"
#include "device/usbd_pvt.h"
#include <cassert>
#include <cstring>

namespace {
constexpr uint16_t ms_os_20_length = 162;
constexpr uint8_t ms_os_20[] = {
    U16_TO_U8S_LE(10), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000), U16_TO_U8S_LE(ms_os_20_length),
    U16_TO_U8S_LE(20), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    U16_TO_U8S_LE(132), U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(7), U16_TO_U8S_LE(42),
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0, 'I', 0, 'n', 0,
    't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0, 'G', 0,
    'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    U16_TO_U8S_LE(80),
    '{', 0, 'F', 0, 'E', 0, '4', 0, '5', 0, '9', 0, 'B', 0, '5', 0,
    '2', 0, '-', 0, '8', 0, '8', 0, '0', 0, '1', 0, '-', 0, '4', 0,
    '4', 0, 'A', 0, 'E', 0, '-', 0, 'B', 0, '5', 0, '7', 0, '9', 0,
    '-', 0, '9', 0, 'E', 0, '3', 0, '1', 0, '8', 0, '9', 0, '5', 0,
    '5', 0, 'D', 0, 'E', 0, '9', 0, 'C', 0, '}', 0, 0, 0, 0, 0
};
static_assert(sizeof(ms_os_20) == ms_os_20_length, "PD-Loader MS OS descriptor length");
constexpr uint8_t bos[] = {
    TUD_BOS_DESCRIPTOR(TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(ms_os_20_length, 1)
};
}

const uint8_t *PDLoaderDevice::bos_descriptor() { return bos; }
const uint8_t *PDLoaderDevice::ms_os_20_descriptor() { return ms_os_20; }
uint16_t PDLoaderDevice::ms_os_20_descriptor_length() { return sizeof(ms_os_20); }

void PDLoaderDevice::initialize() {
    m_epin = next_epin();
    m_epout = next_epout();
    m_strid = next_strid();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);
}

void PDLoaderDevice::process(bool full_poll, bool send_events) {
    if (tud_suspended()) {
        for (auto const &profile : profiles)
            for (auto const &led : profile->leds) led->off();
        return;
    }
    if (!tud_ready()) {
        for (auto const &profile : profiles)
            for (auto const &led : profile->leds) led->update(full_poll, send_events);
        return;
    }
    if (usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    XInputGamepad_Data_t gamepad = {};
    m_input.set_slider_touches(0);
    for (auto const &profile : profiles) {
        for (auto const &mapping : profile->mappings) {
            mapping->update(full_poll, send_events);
            mapping->update_xinput(reinterpret_cast<uint8_t *>(&gamepad));
            mapping->update_pdloader(reinterpret_cast<uint8_t *>(&m_input));
        }
        for (auto const &led : profile->leds) led->update(full_poll, send_events);
    }
    m_input.buttons1 = (gamepad.start << 1) | (gamepad.x << 2) |
        (gamepad.b << 3) | (gamepad.y << 4) | (gamepad.a << 5);
    m_input.buttons2 = (gamepad.leftThumbClick << 6) | ((gamepad.leftTrigger != 0) << 7);
    m_input.buttons3_slider1 = (m_input.buttons3_slider1 & 0xf0) | gamepad.leftShoulder;
    memcpy(m_epin_buffer, &m_input, sizeof(m_input));
    if (usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
        usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, m_epin_buffer, sizeof(m_input), false);
}

size_t PDLoaderDevice::config_descriptor(uint8_t *dest, size_t remaining) {
    const uint8_t descriptor[] = {
        9, TUSB_DESC_INTERFACE, interface_id, 0, 2, TUSB_CLASS_VENDOR_SPECIFIC, 0, 0, m_strid,
        7, TUSB_DESC_ENDPOINT, m_epout, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(PDLOADER_PACKET_SIZE), 1,
        7, TUSB_DESC_ENDPOINT, m_epin, TUSB_XFER_INTERRUPT, U16_TO_U8S_LE(PDLOADER_PACKET_SIZE), 1
    };
    assert(sizeof(descriptor) <= remaining);
    if (sizeof(descriptor) > remaining) return 0;
    memcpy(dest, descriptor, sizeof(descriptor));
    return sizeof(descriptor);
}

size_t PDLoaderDevice::device_name(uint8_t idx, char *desc) {
    if (idx != m_strid) return 0;
    constexpr char name[] = "PD-Loader";
    memcpy(desc, name, sizeof(name));
    return sizeof(name);
}

void PDLoaderDevice::device_descriptor(tusb_desc_device_t *desc) {
    desc->bcdUSB = 0x0210;
    desc->bDeviceClass = TUSB_CLASS_VENDOR_SPECIFIC;
    desc->bDeviceSubClass = 0;
    desc->bDeviceProtocol = 0;
    desc->idVendor = PDLOADER_VID;
    desc->idProduct = PDLOADER_PID;
}

uint16_t PDLoaderDevice::open(tusb_desc_interface_t const *desc, uint16_t max_len) {
    if (desc->bInterfaceClass != TUSB_CLASS_VENDOR_SPECIFIC ||
        desc->bInterfaceSubClass != 0 || desc->bInterfaceProtocol != 0 ||
        desc->bNumEndpoints != 2 || max_len < 23) return 0;
    auto endpoints = tu_desc_next(reinterpret_cast<uint8_t const *>(desc));
    uint8_t epout = 0, epin = 0;
    if (!usbd_open_edpt_pair(TUD_OPT_RHPORT, endpoints, 2, TUSB_XFER_INTERRUPT, &epout, &epin) ||
        epout != m_epout || epin != m_epin) return 0;
    m_led_offset = 0;
    m_led_complete = false;
    if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, m_epout_buffer, sizeof(m_epout_buffer), false))
        return 0;
    return 23;
}

bool PDLoaderDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t bytes) {
    if (ep_addr != m_epout) return true;
    if (result == XFER_RESULT_SUCCESS && bytes <= sizeof(m_epout_buffer)) {
        if (bytes >= 3 && m_epout_buffer[0] == 0x44 &&
            m_epout_buffer[1] == 0x4c && m_epout_buffer[2] == 0x61)
            m_led_offset = 0;
        if (m_led_offset || (bytes >= 3 && m_epout_buffer[0] == 0x44 &&
                             m_epout_buffer[1] == 0x4c && m_epout_buffer[2] == 0x61)) {
            if (bytes <= sizeof(m_led_report) - m_led_offset) {
                memcpy(m_led_staging + m_led_offset, m_epout_buffer, bytes);
                m_led_offset += bytes;
                if (m_led_offset == sizeof(m_led_staging)) {
                    memcpy(m_led_report, m_led_staging, sizeof(m_led_report));
                    m_led_complete = true;
                    m_led_offset = 0;
                }
            } else {
                m_led_offset = 0;
            }
        }
    }
    return usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, m_epout_buffer, sizeof(m_epout_buffer), false);
}

bool PDLoaderDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request) {
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR ||
        request->bmRequestType_bit.direction != TUSB_DIR_IN ||
        request->bRequest != 1 || request->wIndex != 7)
        return false;
    if (stage == CONTROL_STAGE_SETUP)
        return tud_control_xfer(TUD_OPT_RHPORT, request, const_cast<uint8_t *>(ms_os_20), sizeof(ms_os_20));
    return true;
}
