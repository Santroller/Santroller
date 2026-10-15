#include "tusb_option.h"
#include <memory>
#include "managers/profile_manager.hpp"
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "emulation/usb/ogxbox_device.h"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/usb_descriptors.h"
#include "config/config.hpp"
#include "hardware/structs/usb.h"
#include "pico/time.h"

// Debug only: replaces the sticks and triggers with counters showing how the console is talking to us.
// Build with -DOGXBOX_DEBUG_STATS=1, never ship it.
#ifndef OGXBOX_DEBUG_STATS
#define OGXBOX_DEBUG_STATS 0
#endif

#if OGXBOX_DEBUG_STATS
namespace
{
struct DebugCounters
{
    uint32_t loops;
    uint32_t in_done;
    uint32_t out_done;
    uint32_t get_report;
    uint32_t set_report;
    uint32_t other; // vendor (XID descriptor / capabilities) and any other class requests
    uint32_t standard;
    uint32_t other_changes; // times an "other" request differed from the previous one
};
// Counted over the current second, then shown for the next one
DebugCounters debug_counting;
DebugCounters debug_shown;
uint32_t debug_stalls;
// The most recent vendor / unusual class request, which JSRF sends about 60 times a second
tusb_control_request_t debug_last_other;
uint64_t debug_window_start;

void debug_loop()
{
    uint64_t now = time_us_64();
    debug_counting.loops++;
    if (now - debug_window_start >= 1000000)
    {
        debug_shown = debug_counting;
        debug_counting = {};
        debug_window_start = now;
    }
}

// Two counters (clamped to 255) per axis, read back from the raw axis value as
// hi = (raw + 32768) >> 8, lo = (raw + 32768) & 0xFF
int16_t debug_pack(uint32_t hi, uint32_t lo)
{
    return static_cast<int16_t>(static_cast<int32_t>((std::min<uint32_t>(hi, 255) << 8) | std::min<uint32_t>(lo, 255)) - 32768);
}

void debug_count_control(tusb_control_request_t const *request, bool handled)
{
    switch (request->bmRequestType_bit.type)
    {
    case TUSB_REQ_TYPE_STANDARD:
        // eg CLEAR_FEATURE(ENDPOINT_HALT), which a host sends after an error on that endpoint
        debug_counting.standard++;
        break;
    case TUSB_REQ_TYPE_CLASS:
        if (request->bRequest == HID_REQ_CONTROL_GET_REPORT)
            debug_counting.get_report++;
        else if (request->bRequest == HID_REQ_CONTROL_SET_REPORT)
            debug_counting.set_report++;
        else
            debug_counting.other++;
        break;
    default:
        debug_counting.other++;
        break;
    }
    bool is_other = request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR ||
                    (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS &&
                     request->bRequest != HID_REQ_CONTROL_GET_REPORT && request->bRequest != HID_REQ_CONTROL_SET_REPORT);
    if (is_other)
    {
        if (memcmp(&debug_last_other, request, sizeof(debug_last_other)) != 0)
        {
            debug_counting.other_changes++;
        }
        debug_last_other = *request;
    }
    // Standard requests the class declines are answered by tinyusb, so only these really stall
    if (!handled && request->bmRequestType_bit.type != TUSB_REQ_TYPE_STANDARD)
    {
        debug_stalls++;
    }
}

void debug_apply(uint8_t *buf)
{
    auto *report = (OGXboxGamepad_Data_t *)buf;
    // The raw fields of the last "other" request, each read back as v = raw + 32768
    report->leftStickX = debug_pack(debug_last_other.bmRequestType, debug_last_other.bRequest);
    report->leftStickY = static_cast<int16_t>(static_cast<int32_t>(debug_last_other.wValue) - 32768);
    report->rightStickX = debug_pack(debug_last_other.wIndex, debug_last_other.wLength);
    report->rightStickY = debug_pack(debug_shown.other, debug_shown.other_changes);
}
}
#endif

const XID_DESCRIPTOR DukeXIDDescriptor = {
    bLength : sizeof(XID_DESCRIPTOR),
    bDescriptorType : 0x42,
    bcdXid : 0x0100,
    bType : 0x01,
    bSubType : 0x02,
    bMaxInputReportSize : sizeof(OGXboxGamepad_Data_t),
    bMaxOutputReportSize : sizeof(OGXboxOutput_Report_t),
    wAlternateProductIds : {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
};

const OGXboxGamepadCapabilities_Data_t DukeXIDInputCapabilities = {
    rid : 0x00,
    rsize : sizeof(OGXboxGamepadCapabilities_Data_t),
    dpadUp : 1,
    dpadDown : 1,
    dpadLeft : 1,
    dpadRight : 1,
    start : 1,
    back : 1,
    leftThumbClick : 1,
    rightThumbClick : 1,
    padding : 0,
    a : 0xFF,
    b : 0xFF,
    x : 0xFF,
    y : 0xFF,
    leftShoulder : 0xFF,
    rightShoulder : 0xFF,
    leftTrigger : 0xFF,
    rightTrigger : 0xFF,
    leftStickX : UINT16_MAX,
    leftStickY : UINT16_MAX,
    rightStickX : UINT16_MAX,
    rightStickY : UINT16_MAX,
};

const OGXboxOutput_Report_t DukeXIDVibrationCapabilities = {
    rid : 0x00,
    rsize : sizeof(OGXboxOutput_Report_t),
    left : 0xFFFF,
    right : 0xFFFF
};

OGXboxGamepadDevice::OGXboxGamepadDevice()
{
    for (const auto &profile : profiles)
    {
        m_has_slider |= profile->supports_slider;
    }
}
uint16_t OGXboxGamepadDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{

    TU_VERIFY(0x58 == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    // The host has just configured us, so make sure it gets the current state
    m_has_last_report = false;
    m_has_unchanged_frame = false;

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    p_desc = tu_desc_next(p_desc);
    TU_VERIFY(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &m_epout, &m_epin), 0);

    // Prepare for output endpoint
    if (m_epout)
    {
        if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, sizeof(epout_buf), false))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    return drv_len;
}

bool OGXboxGamepadDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
    {
#if OGXBOX_DEBUG_STATS
        if (XFER_RESULT_SUCCESS == result)
        {
            debug_counting.in_done++;
        }
#endif
        return true;
    }
    // Output report
    if (XFER_RESULT_SUCCESS == result)
    {
#if OGXBOX_DEBUG_STATS
        debug_counting.out_done++;
#endif
        if (xferred_bytes >= sizeof(OGXboxOutput_Report_t))
        {
            auto const *report = (OGXboxOutput_Report_t const *)epout_buf;
            set_rumble(report->left >> 8, report->right >> 8);
        }
    }

    // prepare for new transfer
    TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, sizeof(epout_buf), false));
    return true;
}
bool OGXboxGamepadDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    bool handled = control_transfer_impl(stage, request);
#if OGXBOX_DEBUG_STATS
    if (stage == CONTROL_STAGE_SETUP)
    {
        debug_count_control(request, handled);
    }
#endif
    return handled;
}

bool OGXboxGamepadDevice::control_transfer_impl(uint8_t stage, tusb_control_request_t const *request)
{
    // XID uses HID style class requests. Some games poll or rumble over the control endpoint, and
    // older XAPI versions treat a stall as a broken controller, so accept the whole set like the
    // old firmware did.
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS &&
        request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
    {
        switch (request->bRequest)
        {
        case HID_REQ_CONTROL_GET_REPORT:
            if (stage == CONTROL_STAGE_SETUP)
            {
                // Some games (eg JSRF) poll over the control endpoint and barely touch the interrupt
                // endpoint, so build a fresh report here instead of returning the last one sent there
                build_report(ctrl_buf, false, false);
                tud_control_xfer(TUD_OPT_RHPORT, request, ctrl_buf, sizeof(OGXboxGamepad_Data_t));
            }
            return true;
        case HID_REQ_CONTROL_SET_REPORT:
            if (stage == CONTROL_STAGE_SETUP)
            {
                TU_VERIFY(request->wLength <= sizeof(ctrl_buf));
                tud_control_xfer(TUD_OPT_RHPORT, request, ctrl_buf, request->wLength);
            }
            else if (stage == CONTROL_STAGE_DATA && request->wValue == 0x0200 &&
                     request->wLength >= sizeof(OGXboxOutput_Report_t))
            {
                auto const *report = (OGXboxOutput_Report_t const *)ctrl_buf;
                set_rumble(report->left >> 8, report->right >> 8);
            }
            return true;
        case HID_REQ_CONTROL_SET_IDLE:
        case HID_REQ_CONTROL_SET_PROTOCOL:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_status(TUD_OPT_RHPORT, request);
            }
            return true;
        case HID_REQ_CONTROL_GET_IDLE:
        case HID_REQ_CONTROL_GET_PROTOCOL:
            if (stage == CONTROL_STAGE_SETUP)
            {
                // idle rate 0 (only report on change) / report protocol
                static const uint8_t idle_rate = 0;
                static const uint8_t protocol = HID_PROTOCOL_REPORT;
                const uint8_t *value = request->bRequest == HID_REQ_CONTROL_GET_PROTOCOL ? &protocol : &idle_rate;
                tud_control_xfer(TUD_OPT_RHPORT, request, (void *)value, sizeof(*value));
            }
            return true;
        default:
            break;
        }
    }
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
        {
            if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
            {
                if (request->bRequest == 1 && request->wValue == 0x0100)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&DukeXIDInputCapabilities, sizeof(DukeXIDInputCapabilities));
                    }
                    return true;
                }
                if (request->bRequest == 1 && request->wValue == 0x0200)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&DukeXIDVibrationCapabilities, sizeof(DukeXIDVibrationCapabilities));
                    }
                    return true;
                }
                if (request->bRequest == 6 && request->wValue == 0x4200)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&DukeXIDDescriptor, sizeof(DukeXIDDescriptor));
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

void OGXboxGamepadDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);

    memset(&m_initial_report, 0, sizeof(m_initial_report));
    switch (subtype)
    {
    case RockBandDrums:
    {
        OGXboxRockBandDrums_Data_t *report = (OGXboxRockBandDrums_Data_t *)&m_initial_report;
        report->redVelocity = -1;
        report->blueVelocity = -1;
        report->greenVelocity = 0;
        report->yellowVelocity = 0;
        break;
    }
    case GuitarHeroGuitar:
    {
        OGXboxGuitarHeroGuitar_Data_t *report = (OGXboxGuitarHeroGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case RockBandGuitar:
    {
        OGXboxRockBandGuitar_Data_t *report = (OGXboxRockBandGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case LiveGuitar:
    {
        OGXboxGHLGuitar_Data_t *report = (OGXboxGHLGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case GuitarHeroDrums:
    {
        OGXboxGuitarHeroDrums_Data_t *report = (OGXboxGuitarHeroDrums_Data_t *)&m_initial_report;
        report->leftThumbClick = true;
        break;
    }
    default:
        break;
    }
    m_initial_report.rsize = sizeof(OGXboxGamepad_Data_t);
}
void OGXboxGamepadDevice::build_report(uint8_t *buf, bool full_poll, bool send_events)
{
    memcpy(buf, &m_initial_report, sizeof(m_initial_report));
    for (const auto &profile : profiles)
    {
        profile->reset_drum_state();
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_ogxbox(buf);
        }
        for (const auto &led : profile->leds)
        {
            led->update(full_poll, send_events);
        }
    }
    if (subtype == GuitarHeroGuitar && m_has_slider)
    {
        // convert bitmask slider to actual hid slider
        OGXboxGuitarHeroGuitar_Data_t *reportGh = (OGXboxGuitarHeroGuitar_Data_t *)buf;
        reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
    }
#if OGXBOX_DEBUG_STATS
    debug_apply(buf);
#endif
}

void OGXboxGamepadDevice::process(bool full_poll, bool send_events)
{
#if OGXBOX_DEBUG_STATS
    debug_loop();
#endif
    if (tud_suspended()) {
        process_suspended(full_poll, send_events);
        return;
    }
    if (!tud_ready())
    {
        for (const auto &profile : profiles)
        {
            for (const auto &led : profile->leds)
            {
                led->update(full_poll, send_events);
            }
        }
        return;
    }
    if (!tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    uint16_t frame = usb_hw->sof_rd & USB_SOF_RD_BITS;
    if (m_has_unchanged_frame && frame == m_unchanged_frame)
        return;
    build_report(epin_buf, full_poll, send_events);

    if (m_has_last_report && memcmp(&m_last_report, epin_buf, sizeof(OGXboxGamepad_Data_t)) == 0)
    {
        m_unchanged_frame = frame;
        m_has_unchanged_frame = true;
        return;
    }
    m_has_unchanged_frame = false;

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    if (usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(OGXboxGamepad_Data_t), false))
    {
        memcpy(&m_last_report, epin_buf, sizeof(OGXboxGamepad_Data_t));
        m_has_last_report = true;
    }
    else
    {
        usbd_edpt_release(TUD_OPT_RHPORT, m_epin);
    }
}

size_t OGXboxGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t OGXboxGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_OGXBOX_GAMEPAD_DESCRIPTOR(interface_id, m_epin, m_epout)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t OGXboxGamepadDevice::device_name(uint8_t idx, char *desc) 
{
    return 0;
}

void OGXboxGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x045E;
    desc->idProduct = 0x0289;
}
