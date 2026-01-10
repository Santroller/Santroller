#include "usb/device/ps5_device.h"
#include "protocols/ps5.hpp"
#include "enums.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "hid_reports.h"

static const int ps5_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};
uint8_t ps5_feature_config[] = {
    0x03, 0x21, 0x27, 0x04, 0x91, /*type*/ 0x00, 0x2c, 0x56,
    0xa0, 0x0f, 0x3d, 0x00, 0x00, 0x04, 0x01, 0x00,
    0x00, 0x20, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

AuthPageSizeReport ps5_pagesize_report = {
    type : 0xF3,
    u1 : 0x00,
    size_challenge : 0x38,
    size_response : 0x38,
    u4 : {0x00, 0x00, 0x00, 0x00}
};
uint8_t const desc_hid_report_ps5[] =
    {
        TUD_HID_REPORT_DESC_PS4_FIRSTPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

PS5GamepadDevice::PS5GamepadDevice()
{
}
void PS5GamepadDevice::initialize()
{
}
void PS5GamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    for (const auto &mapping : mappings)
    {
        mapping->update(full_poll);
        // TODO
        // mapping->update_ps5(epin_buf);
    }
    PS5Gamepad_Data_t *gamepad = (PS5Gamepad_Data_t *)epin_buf;
    gamepad->report_id = 1;
    gamepad->leftStickX = PS3_STICK_CENTER;
    gamepad->leftStickY = PS3_STICK_CENTER;
    gamepad->rightStickX = PS3_STICK_CENTER;
    gamepad->rightStickY = PS3_STICK_CENTER;
    // convert bitmask dpad to actual hid dpad
    gamepad->dpad = dpad_bindings[gamepad->dpad];

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(PS5Gamepad_Data_t));
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
    }
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(m_interface, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps5), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void PS5GamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x0c70;
    desc->idProduct = 0x0777;
}
const uint8_t *PS5GamepadDevice::report_descriptor()
{
    return desc_hid_report_ps5;
}

uint16_t PS5GamepadDevice::report_desc_len() {
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
        case ReportId::ReportIdPs4SetChallenge:
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
            }
        }
        break;
    }
    default:
        break;
    }
}