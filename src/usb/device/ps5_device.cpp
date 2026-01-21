#include "usb/device/ps5_device.h"
#include "protocols/ps5.hpp"
#include "enums.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "hid_reports.h"
#include "usb/usb_devices.h"

static const int ps5_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};

uint8_t ps5_feature_config[] = {
    0x03,
    0x21,
    0x28,
    0x03,
    0xC3,
    /*type*/ 0x00,
    0x2C,
    0x56,
    0x01,
    0x00,
    0xD0,
    0x07,
    0x00,
    0x80,
    0x04,
    0x00,
    0x00,
    0x80,
    0x0D,
    0x0D,
    0x84,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

uint8_t const desc_hid_report_ps5[] =
    {
        TUD_HID_REPORT_DESC_PS5_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

PS5GamepadDevice::PS5GamepadDevice()
{
}
void PS5GamepadDevice::initialize()
{
}
void PS5GamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || !m_eps_assigned || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    PS5Dpad_Data_t *gamepad = (PS5Dpad_Data_t *)epin_buf;
    gamepad->report_id = 1;
    gamepad->leftStickX = PS3_STICK_CENTER;
    gamepad->leftStickY = PS3_STICK_CENTER;
    gamepad->rightStickX = PS3_STICK_CENTER;
    gamepad->rightStickY = PS3_STICK_CENTER;
    for (const auto &mapping : mappings)
    {
        mapping->update(full_poll);
        mapping->update_ps5(epin_buf);
    }
    // convert bitmask dpad to actual hid dpad
    gamepad->dpad = GamepadButtonMapping::dpad_bindings[gamepad->dpad];

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(PS5Dpad_Data_t));
}

size_t PS5GamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t PS5GamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epin();
        usb_instances_by_epnum[m_epin] = usb_instances[interface_id];
        usb_instances_by_epnum[m_epout] = usb_instances[interface_id];
    }
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps5), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void PS5GamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = P5GENERAL_VENDOR_ID;
    desc->idProduct = P5GENERAL_PRODUCT_ID;
}
const uint8_t *PS5GamepadDevice::report_descriptor()
{
    return desc_hid_report_ps5;
}

uint16_t PS5GamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_ps5);
}

uint16_t PS5GamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    if (report_type != HID_REPORT_TYPE_FEATURE)
    {
        return 0;
    }

    switch (report_id)
    {
    case ReportId::ReportIdPs5Feature:
        memcpy(buffer, ps5_feature_config, sizeof(ps5_feature_config));
        switch (current_type)
        {
        case Gamepad:
            buffer[5] = PS5_GAMEPAD;
            break;
        case GuitarHeroGuitar:
        case RockBandGuitar:
        case LiveGuitar:
            buffer[5] = PS5_GUITAR;
            break;
        case GuitarHeroDrums:
        case RockBandDrums:
            buffer[5] = PS5_DRUMS;
            break;
        case FightStick:
            buffer[5] = PS5_FIGHTSTICK;
            break;
        default:
            break;
        }
        return sizeof(ps5_feature_config);
    case ReportId::ReportIdPs5GetResponse:
        // try to pass through to ps4 over usb host if one exists
        return 0;
    case ReportId::ReportIdPs5GetAuthStatus:
        // try to pass through to ps4 over usb host if one exists
        return 0;
    }
    return 0;
}

void PS5GamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
    {
        switch (report_id)
        {
        case 0:
            return;
        case ReportId::ReportIdPs5SetChallenge:
            // TODO: pass to ps5 controller for auth
            return;
        }
        break;
    }
    case HID_REPORT_TYPE_OUTPUT:
    {
        ps4_output_report *report = (ps4_output_report *)buffer;
        lightbar_red = report->lightbar_red;
        lightbar_green = report->lightbar_green;
        lightbar_blue = report->lightbar_blue;
        rumble_left = report->motor_left;
        rumble_right = report->motor_right;
        for (int i = 0; i < 4; i++)
        {
            if (report->lightbar_red == ps5_colors[i][0] && report->lightbar_green == ps5_colors[i][1] && report->lightbar_blue == ps5_colors[i][2])
            {
                player_led = i + 1;
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}