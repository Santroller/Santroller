#include "tusb_option.h"
#include <memory>
#include "managers/profile_manager.hpp"

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "emulation/usb/xinput_device.h"
#include "managers/config_manager.hpp"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/usb_descriptors.h"
#include <xsm3.h>
#include "usb/auth_broker.h"

// Set once the console has sent its verify challenge, for spotting when auth finishes
static bool s_security_verified = false;
#include <pico/unique_id.h>
#include "emulation/usb/usb_devices.h"
#include "config/config.hpp"
#include "hardware/structs/usb.h"
#include "utils.h"
static const char str_xb360_auth[] = "Xbox Security Method 3, Version 1.00, \xa9 2005 Microsoft Corporation. All rights reserved.";
static const uint8_t xbox_players[] = {
    0, // 0x00	 All off
    0, // 0x01	 All blinking
    1, // 0x02	 1 flashes, then on
    2, // 0x03	 2 flashes, then on
    3, // 0x04	 3 flashes, then on
    4, // 0x05	 4 flashes, then on
    1, // 0x06	 1 on
    2, // 0x07	 2 on
    3, // 0x08	 3 on
    4, // 0x09	 4 on
    0, // 0x0A	 Rotating (e.g. 1-2-4-3)
    0, // 0x0B	 Blinking*
    0, // 0x0C	 Slow blinking*
    0, // 0x0D	 Alternating (e.g. 1+4-2+3), then back to previous*
};
void XInputGamepadDevice::reset()
{
    memset(xinputInterfaces, 0xFF, sizeof(xinputInterfaces));
    lastIntfInput = 0;
    last_caps_query_time = 0;
    lastIntfSerial = 0;
    last_serial_query_time = 0;
    ++report_generation;
}
XInputGamepadDevice::XInputGamepadDevice()
{
    reset();
}
void XInputGamepadDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    m_strid = next_strid();
    ProfileManager::instance().map_usb_instance_epin(m_epin, interface_id);
    ProfileManager::instance().map_usb_instance_epout(m_epout, interface_id);

    memset(&m_initial_report, 0, sizeof(m_initial_report));
    memset(&m_last_report, 0, sizeof(m_last_report));
    m_has_last_report = false;
    m_has_unchanged_frame = false;
    m_report_generation = report_generation;
    m_initial_report.leftStickX = 0;
    m_initial_report.leftStickY = 0;
    m_initial_report.rightStickX = 0;
    m_initial_report.rightStickY = 0;
    switch (subtype)
    {
    case RockBandDrums:
    {
        XInputRockBandDrums_Data_t *report = (XInputRockBandDrums_Data_t *)&m_initial_report;
        report->redVelocity = -1;
        report->blueVelocity = -1;
        report->greenVelocity = 0;
        report->yellowVelocity = 0;
        break;
    }
    case GuitarHeroGuitar:
    {
        XInputGuitarHeroGuitar_Data_t *report = (XInputGuitarHeroGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case RockBandGuitar:
    {
        XInputRockBandGuitar_Data_t *report = (XInputRockBandGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case LiveGuitar:
    {
        XInputGHLGuitar_Data_t *report = (XInputGHLGuitar_Data_t *)&m_initial_report;
        report->whammy = INT16_MIN;
        break;
    }
    case GuitarHeroDrums:
    {
        XInputGuitarHeroDrums_Data_t *report = (XInputGuitarHeroDrums_Data_t *)&m_initial_report;
        report->leftThumbClick = true;
        break;
    }
    default:
        break;
    }
    m_initial_report.rsize = sizeof(XInputGamepad_Data_t);
}

uint16_t XInputGamepadDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{

    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceSubClass == 0x5D &&
        (itf_desc->bInterfaceProtocol == 0x01 ||
         itf_desc->bInterfaceProtocol == 0x03 ||
         itf_desc->bInterfaceProtocol == 0x02 ||
         itf_desc->bInterfaceProtocol == 0x81))
    {
        const bool wireless = itf_desc->bInterfaceProtocol == 0x81;
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_VERIFY((wireless ? XINPUT_DESC_TYPE_WIRELESS_CAPABILITIES : XINPUT_DESC_TYPE_RESERVED) == x_desc->bDescriptorType, 0);
        m_wireless = wireless;
        // A genuine receiver reports a controller linking without being asked
        m_send_presence = wireless;
        m_send_link = wireless;
        m_send_caps = false;
        drv_len += x_desc->bLength;
        p_desc = tu_desc_next(p_desc);
        TU_VERIFY(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, itf_desc->bNumEndpoints,
                                      TUSB_XFER_INTERRUPT, &m_epout,
                                      &m_epin),
                  0);
        if (m_epout)
        {
            if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, sizeof(epout_buf), false))
            {
                TU_LOG_FAILED();
                TU_BREAKPOINT();
            }
        }
        bool already_added = false;
        for (size_t i = 0; i < sizeof(xinputInterfaces); i++)
        {
            if (xinputInterfaces[i] == itf_desc->bInterfaceNumber)
            {
                already_added = true;
                break;
            }
        }
        if (!already_added)
        {
            for (size_t i = 0; i < sizeof(xinputInterfaces); i++)
            {
                if (xinputInterfaces[i] == 0xFF)
                {
                    xinputInterfaces[i] = itf_desc->bInterfaceNumber;
                    printf("assigned: %02x, %02x\r\n", i, itf_desc->bInterfaceNumber);
                    break;
                }
            }
        }
        return drv_len;
    }
    return 0;
}
bool XInputGamepadDevice::s_wake_guide_pending = false;
bool XInputGamepadDevice::s_wake_guide_held = false;
uint32_t XInputGamepadDevice::s_wake_guide_since = 0;
// Like a deliberate press of a real guide button, as a quick one doesn't turn the console on. Kept short of
// the hold that brings up the turn off prompt on a console that's already on
static constexpr uint32_t WAKE_GUIDE_MS = 1000;
// How long after signalling resume the console has to start polling again for it to get the guide press
static constexpr uint32_t WAKE_GUIDE_EXPIRE_MS = 5000;

void XInputGamepadDevice::hold_guide_for_wake()
{
    s_wake_guide_pending = true;
    s_wake_guide_since = millis();
}

void XInputGamepadDevice::process(bool full_poll, bool send_events)
{
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
    if (m_wireless && send_wireless_reply())
        return;
    // Build at most one report per frame (the host polls every frame), even when an unchanged
    // report isn't sent. Otherwise the mappings would run every loop instead of once per poll,
    // which time based logic like the cymbal glitch fix isn't written for.
    uint16_t frame = usb_hw->sof_rd & USB_SOF_RD_BITS;
    if (m_has_unchanged_frame && frame == m_unchanged_frame)
        return;
    update_stagekit();
    memcpy(epin_buf, &m_initial_report, sizeof(m_initial_report));
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)epin_buf;
    // Profiles are attached after construction, so this can't be cached in the constructor
    bool has_slider = false;
    for (const auto &profile : profiles)
    {
        has_slider |= profile->supports_slider;
        profile->reset_drum_state();
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(full_poll, send_events);
            mapping->update_xinput(epin_buf);
        }
        for (const auto &led : profile->leds)
        {
            // In standby the 360 keeps polling, but the LEDs go off like they would for a suspended host
            if (host_in_standby())
            {
                led->off();
            }
            else
            {
                led->update(full_poll, send_events);
            }
        }
    }
    if (s_wake_guide_pending)
    {
        s_wake_guide_pending = false;
        // A console that ignored the resume shouldn't get a guide press whenever it's next turned on
        s_wake_guide_held = millis() - s_wake_guide_since < WAKE_GUIDE_EXPIRE_MS;
        s_wake_guide_since = millis();
    }
    if (s_wake_guide_held && millis() - s_wake_guide_since < WAKE_GUIDE_MS)
    {
        report->guide = 1;
    }
    else
    {
        s_wake_guide_held = false;
    }
    if (subtype == GuitarHeroGuitar && has_slider)
    {
        // convert bitmask slider to actual hid slider
        XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)report;
        reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
    }

    if (m_report_generation != report_generation)
    {
        m_report_generation = report_generation;
        m_has_last_report = false;
    }

    if (m_has_last_report &&
        memcmp(&m_last_report, epin_buf, sizeof(XInputGamepad_Data_t)) == 0)
    {
        // Nothing to send, so wait for the next frame before building another report
        m_unchanged_frame = frame;
        m_has_unchanged_frame = true;
        return;
    }
    m_has_unchanged_frame = false;

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }

    memcpy(&m_last_report, epin_buf, sizeof(XInputGamepad_Data_t));
    uint16_t len = sizeof(XInputGamepad_Data_t);
    if (m_wireless)
    {
        // A receiver wraps the wired report, after its id and size, as 00 01 00 f0 00 13 <report>
        memmove(epin_buf + 6, epin_buf + 2, sizeof(XInputGamepad_Data_t) - 2);
        static const uint8_t header[] = {0x00, 0x01, 0x00, 0xf0, 0x00, 0x13};
        memcpy(epin_buf, header, sizeof(header));
        len = XINPUT_WIRELESS_PACKET_SIZE;
        memset(epin_buf + 6 + sizeof(XInputGamepad_Data_t) - 2, 0, len - (6 + sizeof(XInputGamepad_Data_t) - 2));
    }
    // m_last_report already holds this report, so it only counts as sent if the transfer started
    m_has_last_report = usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, len, false);
    if (!m_has_last_report)
    {
        usbd_edpt_release(TUD_OPT_RHPORT, m_epin);
    }
}

void XInputGamepadDevice::handle_led(uint8_t led)
{
    if (led < sizeof(xbox_players))
    {
        uint8_t player = xbox_players[led];
        if (player || led == 0x00)
        {
            set_player_led(player);
        }
    }
}

void XInputGamepadDevice::handle_rumble(uint8_t left, uint8_t right)
{
    if (subtype == DjHeroTurntable && left == right)
    {
        set_euphoria_led(left);
    }
    else if (subtype_supports_stagekit(subtype))
    {
        process_stagekit_command(right, left);
    }
    else
    {
        set_rumble(left, right);
    }
}

// Commands xusb22 sends a receiver slot, from a capture of a genuine receiver
void XInputGamepadDevice::process_wireless_command(const uint8_t *buf, uint32_t len)
{
    if (len < 4)
    {
        return;
    }
    if (buf[0] == 0x08 && buf[2] == 0x0f && buf[3] == 0xc0)
    {
        // is anything linked to this slot
        m_send_presence = true;
        m_send_link = true;
    }
    else if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x02 && buf[3] == 0x80)
    {
        m_send_caps = true;
    }
    else if (buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x08 && (buf[3] & 0xf0) == 0x40)
    {
        // same codes as the wired LED command
        handle_led(buf[3] & 0x0f);
    }
    else if (buf[0] == 0x00 && buf[1] == 0x01 && buf[2] == 0x0f && buf[3] == 0xc0 && len >= 7)
    {
        handle_rumble(buf[5], buf[6]);
    }
}

bool XInputGamepadDevice::send_wireless_reply()
{
    if (!m_send_presence && !m_send_link && !m_send_caps)
    {
        return false;
    }
    memset(epin_buf, 0, XINPUT_WIRELESS_PACKET_SIZE);
    uint16_t len = XINPUT_WIRELESS_PACKET_SIZE;
    if (m_send_presence)
    {
        // gamepad linked, no headset
        epin_buf[0] = 0x08;
        epin_buf[1] = 0x80;
        len = 2;
    }
    else if (m_send_link)
    {
        uint8_t receiver[7];
        receiver_id(receiver);
        const uint32_t serial = controller_serial(interface_id);
        // The bytes between the serial and the subtype are what a genuine controller sent
        const uint8_t link[] = {0x00, 0x0f, 0x00, 0xf0, 0xf0, 0xcc,
                                receiver[0], receiver[1], receiver[2], receiver[3],
                                (uint8_t)serial, (uint8_t)(serial >> 8), (uint8_t)(serial >> 16), (uint8_t)(serial >> 24),
                                0x00, 0x00, 0x13, 0xe3, 0x20, 0x1d, 0x30, 0x03, 0x44, 0x30, 0x51,
                                get_xinput_subtype(subtype), 0xff, 0xff, 0xff};
        static_assert(sizeof(link) == XINPUT_WIRELESS_PACKET_SIZE);
        memcpy(epin_buf, link, sizeof(link));
    }
    else
    {
        const auto caps = input_capabilities();
        const uint8_t header[] = {0x00, 0x05, 0x00, 0xf0, 0x00, 0x12};
        memcpy(epin_buf, header, sizeof(header));
        // buttons through the right stick, as in the wired report
        memcpy(epin_buf + sizeof(header), &caps.buttons, 12);
        epin_buf[18] = 0xff; // left motor
        epin_buf[19] = 0xff; // right motor
        memcpy(epin_buf + 20, &caps.flags, sizeof(caps.flags));
    }
    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return true;
    }
    if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, len, false))
    {
        usbd_edpt_release(TUD_OPT_RHPORT, m_epin);
        return true;
    }
    if (m_send_presence)
    {
        m_send_presence = false;
    }
    else if (m_send_link)
    {
        m_send_link = false;
    }
    else
    {
        m_send_caps = false;
    }
    // a new report has to go out after these, even if the inputs haven't changed
    m_has_last_report = false;
    return true;
}

// The first controller claimed by the profiles behind the pad on itf that identifies itself
static std::optional<uint64_t> pad_hardware_id(uint8_t itf)
{
    auto instance = ProfileManager::instance().get_usb_instance(itf);
    if (!instance)
    {
        return std::nullopt;
    }
    for (const auto &profile : instance->profiles)
    {
        if (!profile)
        {
            continue;
        }
        for (const auto &[slot, device] : profile->claimed_devices)
        {
            if (!device)
            {
                continue;
            }
            if (auto id = device->hardware_id())
            {
                return id;
            }
        }
    }
    return std::nullopt;
}

uint32_t XInputGamepadDevice::controller_serial(uint8_t itf)
{
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    // FNV-1a
    uint32_t hash = 2166136261u;
    for (const auto byte : id.id)
    {
        hash = (hash ^ byte) * 16777619u;
    }
    // Follow the controller driving this pad, so the console sees the same controller whichever pad it is
    // on. 0xff is the receiver, which has no controller
    const auto hardware_id = itf == 0xff ? std::nullopt : pad_hardware_id(itf);
    if (!hardware_id)
    {
        return (hash ^ itf) * 16777619u;
    }
    // One controller can drive more than one pad. Only the first gets its serial, the console would
    // treat the rest as the same pad
    for (const auto other : xinputInterfaces)
    {
        if (other == 0xFF || other == itf)
        {
            break;
        }
        if (pad_hardware_id(other) == hardware_id)
        {
            return (hash ^ itf) * 16777619u;
        }
    }
    const uint64_t id_value = *hardware_id;
    for (int i = 0; i < 8; i++)
    {
        hash = (hash ^ static_cast<uint8_t>(id_value >> (i * 8))) * 16777619u;
    }
    return hash;
}

void XInputGamepadDevice::receiver_id(uint8_t id[7])
{
    // board specific, with the tail a genuine receiver sent
    const uint32_t serial = controller_serial(0xff);
    const uint8_t receiver[7] = {(uint8_t)serial, (uint8_t)(serial >> 8), (uint8_t)(serial >> 16), (uint8_t)(serial >> 24),
                                 0x08, 0x0d, 0x13};
    memcpy(id, receiver, sizeof(receiver));
}

bool XInputGamepadDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
    {
        return true;
    }

    if (XFER_RESULT_SUCCESS == result && m_wireless)
    {
        process_wireless_command(epout_buf, xferred_bytes);
        TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, 0x20, false));
        return true;
    }
    if (XFER_RESULT_SUCCESS == result)
    {
        XInputLEDReport_t const *ledReport = (XInputLEDReport_t const *)epout_buf;
        XInputRumbleReport_t const *rumbleReport = (XInputRumbleReport_t const *)epout_buf;
        if (ledReport->rid == XBOX_LED_ID)
        {
            handle_led(ledReport->led);
        }
        else if (rumbleReport->rid == XBOX_RUMBLE_ID)
        {
            handle_rumble(rumbleReport->leftRumble, rumbleReport->rightRumble);
        }
        TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, 0x20, false));
    }
    return true;
}
bool XInputGamepadDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
        {
            if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE && request->bRequest == HID_REQ_CONTROL_GET_REPORT)
            {
                if (request->wValue == VIBRATION_CAPABILITIES_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        printf("vibr\r\n");
                        XInputVibrationCapabilities_t XInputVibrationCapabilities = {
                            rid : 0x00,
                            rsize : sizeof(XInputVibrationCapabilities_t),
                            padding : 0x00,
                            left_motor : 0xFF,
                            right_motor : 0xFF,
                            padding_2 : {0x00, 0x00, 0x00}
                        };
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&XInputVibrationCapabilities, sizeof(XInputVibrationCapabilities_t));
                    }
                    return true;
                }
                else if (request->wValue == INPUT_CAPABILITIES_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        printf("caps\r\n");
                        XInputInputCapabilities_t XInputInputCapabilities = input_capabilities();
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&XInputInputCapabilities, sizeof(XInputInputCapabilities_t));
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

XInputInputCapabilities_t XInputGamepadDevice::input_capabilities() const
{
        XInputInputCapabilities_t XInputInputCapabilities = {
            rid : 0x00,
            rsize : sizeof(XInputInputCapabilities_t),
            buttons : 0xf73f,
            leftTrigger : 0xff,
            rightTrigger : 0xff,
            leftThumbX : USB_VID,
            leftThumbY : USB_PID,
            rightThumbX : (uint16_t)(USB_VERSION_BCD(subtype, 0, 0)),
            rightThumbY : 0xffc0,
            reserved : {0x00, 0x00, 0x00, 0x00},
            flags : XINPUT_FLAGS_FORCE_FEEDBACK
        };
        switch (subtype)
        {
        case LiveGuitar:
            XInputInputCapabilities.flags = XINPUT_FLAGS_NO_NAV;
            break;
        case ProGuitarMustang:
            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
            XInputInputCapabilities.leftThumbX = HARMONIX_VID;
            XInputInputCapabilities.leftThumbY = XBOX_360_MUSTANG_PID;
            break;
        case ProGuitarSquier:
            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
            XInputInputCapabilities.leftThumbX = HARMONIX_VID;
            XInputInputCapabilities.leftThumbY = XBOX_360_SQUIER_PID;
            break;
        case GuitarHeroDrums:
            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
            break;
        case RockBandDrums:
            // kick 2 and the pad flag, like the MPA
            XInputInputCapabilities.buttons = 0xf7ff;
            break;
        case DjHeroTurntable:
            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
            break;
        default:
            break;
        }
        XInputInputCapabilities.rightThumbX = USB_VERSION_BCD(subtype, 0, 0);
        return XInputInputCapabilities;
}

size_t XInputGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : interface_id,
        Reserved : 0x01,
        CompatibleID : "XUSB10",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    if (ProfileManager::instance().xinput_receiver())
    {
        // xusb22's receiver slot driver
        memcpy(section.CompatibleID, "XUSB20", sizeof("XUSB20"));
    }
    assert(sizeof(section) <= remaining);
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t XInputGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    lastIntfInput = 0;
    last_caps_query_time = 0;
    lastIntfSerial = 0;
    last_serial_query_time = 0;
    if (ProfileManager::instance().xinput_receiver())
    {
        uint8_t desc[] = {TUD_XINPUT_WIRELESS_CONTROLLER_DESCRIPTOR(interface_id, m_epin, m_epout, m_strid)};
        assert(sizeof(desc) <= remaining);
        memcpy(dest, desc, sizeof(desc));
        return sizeof(desc);
    }
    uint8_t desc[] = {TUD_XINPUT_GAMEPAD_DESCRIPTOR(interface_id, m_epin, m_epout, get_xinput_subtype(subtype))};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t XInputGamepadDevice::device_name(uint8_t idx, char *desc)
{
    if (idx == m_strid)
    {
        memcpy(desc, profiles[0]->name, sizeof(profiles[0]->name));
        return sizeof(profiles[0]->name);
    }
    return 0;
}

void XInputGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
}

XInputSecurityDevice::XInputSecurityDevice()
{
}
void XInputSecurityDevice::initialize()
{
    m_strid = next_strid();
}
void XInputSecurityDevice::process(bool full_poll, bool send_events)
{
}

size_t XInputSecurityDevice::device_name(uint8_t idx, char *desc)
{

    if (m_strid != idx)
    {
        return 0;
    }
    memcpy(desc, str_xb360_auth, sizeof(str_xb360_auth));
    return sizeof(str_xb360_auth);
}

uint16_t XInputSecurityDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{

    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceSubClass == 0xfD &&
        itf_desc->bInterfaceProtocol == 0x13)
    {
        // xinput security interface
        p_desc = tu_desc_next(p_desc);
        XBOX_SECURITY_DESCRIPTOR *x_desc =
            (XBOX_SECURITY_DESCRIPTOR *)p_desc;
        TU_VERIFY(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        return drv_len;
    }
    return 0;
}

size_t XInputSecurityDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : interface_id,
        Reserved : 0x01,
        CompatibleID : "WINUSB",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    assert(sizeof(section) <= remaining);
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t XInputSecurityDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_XINPUT_SECURITY_DESCRIPTOR(interface_id, m_strid)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void XInputSecurityDevice::device_descriptor(tusb_desc_device_t *desc)
{
}

bool XInputSecurityDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    return false;
}
bool XInputSecurityDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    static uint8_t buf[0x22];

    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS)
    {
        switch (request->bRequest)
        {
        // RPCS3 will send this, so jump to PS3 mode if the profile wants passthrough there
        case HID_REQ_CONTROL_GET_REPORT:
            if (stage == CONTROL_STAGE_SETUP)
            {
                uint8_t const report_id = tu_u16_low(request->wValue);
                if (report_id == 0xF2 && ProfileManager::instance().ps3_on_rpcs3())
                    ConfigManager::instance().rpcs3_detected();
            }
            break;
        }
    }
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        switch (request->bRequest)
        {
        case 0x81:
            if (stage == CONTROL_STAGE_SETUP)
            {
                uint8_t serial[0x0C];
                pico_get_unique_board_id_string((char *)serial, sizeof(serial));
                serial[0] = to_us_since_boot(get_absolute_time());
                xsm3_set_vid_pid(serial, USB_VID, USB_PID);
                xsm3_initialise_state();
                xsm3_set_identification_data(xsm3_id_data_ms_controller);
                tud_control_xfer(TUD_OPT_RHPORT, request, xsm3_id_data_ms_controller, sizeof(xsm3_id_data_ms_controller));
                // a new handshake starting
                s_security_verified = false;
                auth_broker.set_auth_completed(ModeXbox360, false);
            }
                    ConfigManager::instance().host_detected(ModeXbox360);
            return true;
        case 0x82:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                xsm3_do_challenge_init(buf);
            }
            return true;
        case 0x87:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                xsm3_do_challenge_verify(buf);
                s_security_verified = true;
            }
            return true;
        case 0x83:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(TUD_OPT_RHPORT, request, xsm3_challenge_response, sizeof(xsm3_challenge_response));
                // the console reading the response to its verify challenge is the last step
                if (s_security_verified)
                {
                    auth_broker.set_auth_completed(ModeXbox360, true);
                }
            }
            return true;
        case 0x86:
            if (stage == CONTROL_STAGE_SETUP)
            {
                short state = 2; // 1 = in-progress, 2 = complete
                tud_control_xfer(TUD_OPT_RHPORT, request, &state, sizeof(state));
            }
            return true;
        }
    }
    else
    {
        if (stage == CONTROL_STAGE_DATA || stage == CONTROL_STAGE_SETUP)
        {
            switch (request->bRequest)
            {
            case 0x82:
                if (stage == CONTROL_STAGE_DATA)
                {
                    xsm3_do_challenge_init(buf);
                }
                else
                {
                    tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                }
                return true;
            case 0x87:
                if (stage == CONTROL_STAGE_DATA)
                {
                    xsm3_do_challenge_verify(buf);
                    s_security_verified = true;
                }
                else
                {
                    tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                }
                return true;
            case 0x84:
                if (stage == CONTROL_STAGE_SETUP)
                {
                    tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                }
                return true;
            }
        }
    }

    return false;
}

uint8_t XInputGamepadDevice::xinputInterfaces[4] = {0xFF, 0xFF, 0xFF, 0xFF};
uint8_t XInputGamepadDevice::lastIntfInput = 0;
uint32_t XInputGamepadDevice::last_caps_query_time = 0;
uint8_t XInputGamepadDevice::lastIntfSerial = 0;
uint32_t XInputGamepadDevice::last_serial_query_time = 0;
volatile uint32_t XInputGamepadDevice::report_generation = 0;