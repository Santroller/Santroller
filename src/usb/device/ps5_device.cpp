#include "usb/device/ps5_device.h"
#include "protocols/ps5.hpp"
#include "enums.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "hid_reports.h"
#include "usb/usb_devices.h"
#include "usb/host/hid_host.h"

static const int ps5_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};

uint8_t ps5_feature_config[] = {
    0x03, 0x21, 0x28, 0x03, 0xC3, 0x00 /*type*/, 0x2C, 0x56,
    0x01, 0x00, 0xD0, 0x07, 0x00, 0x80, 0x04, 0x00,
    0x00, 0x80, 0x0D, 0x0D, 0x84, 0x00, 0x00, 0x00,
    0x00 /*extended type*/, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t const desc_hid_report_ps5[] =
    {
        TUD_HID_REPORT_DESC_PS5_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

PS5GamepadDevice::PS5GamepadDevice()
{
}
void PS5GamepadDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    usb_instances_by_epin[m_epin & (~0x80)] = usb_instances[interface_id];
    usb_instances_by_epout[m_epout] = usb_instances[interface_id];
    PS5Dpad_Data_t *gamepad = (PS5Dpad_Data_t *)initialReport;
    memset(initialReport, 0, sizeof(initialReport));
    gamepad->report_id = 1;
    gamepad->leftStickX = PS3_STICK_CENTER;
    gamepad->leftStickY = PS3_STICK_CENTER;
    gamepad->rightStickX = PS3_STICK_CENTER;
    gamepad->rightStickY = PS3_STICK_CENTER;
    gamepad->data_30_31_0x001a = 0x001a;
}
void PS5GamepadDevice::process()
{
    if (!ready() || !got_feature)
        return;
    memcpy(epin_buf, &initialReport, sizeof(initialReport));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
            mapping->update_ps5(epin_buf);
        }
        for (const auto &led : profile->leds)
        {
            led->update(false, false);
        }
    }
    PS5Dpad_Data_t *gamepad = (PS5Dpad_Data_t *)epin_buf;
    // convert bitmask dpad to actual hid dpad
    gamepad->dpad = GamepadButtonMapping::dpad_bindings[gamepad->dpad];
    std::shared_ptr<HidHost> host_device;
    auto auth_device = auth_devices.find(ModePs5);
    if (auth_device != auth_devices.end())
    {
        host_device = std::static_pointer_cast<HidHost>(auth_device->second);
        if (m_report_ready)
        {
            m_report_ready = false;
            host_device->send_intr_report(epin_buf, sizeof(PS5Dpad_Data_t));
        }
        if (host_device->get_intr_report(epin_buf, sizeof(epin_buf)))
        {
            send_report(sizeof(PS5Dpad_Data_t), 0, epin_buf);
            m_report_ready = true;
        }
    }
}

size_t PS5GamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t PS5GamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps5), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t PS5GamepadDevice::device_name(uint8_t idx, char *desc) 
{
    return 0;
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

    std::shared_ptr<HidHost> host_device;
    auto auth_device = auth_devices.find(ModePs5);
    if (auth_device != auth_devices.end())
    {
        host_device = std::static_pointer_cast<HidHost>(auth_device->second);
    }

    switch (report_id)
    {
    case ReportId::ReportIdPs5Feature:
        memcpy(buffer, ps5_feature_config, sizeof(ps5_feature_config));
        switch (subtype)
        {
        case Gamepad:
            buffer[5] = PS5_GAMEPAD;
            break;
        case GuitarHeroGuitar:
        case LiveGuitar:
            buffer[5] = PS5_GUITAR;
            buffer[24] = 0x06;
            break;
        case RockBandGuitar:
            buffer[5] = PS5_GUITAR;
            buffer[24] = 0x07;
            break;
        case GuitarHeroDrums:
            buffer[5] = PS5_DRUMS;
            buffer[24] = 0x1f;
            break;
        case RockBandDrums:
            buffer[5] = PS5_DRUMS;
            buffer[24] = 0x1f;
            break;
        case FightStick:
            buffer[5] = PS5_FIGHTSTICK;
            break;
        default:
            break;
        }
        got_feature = true;
        return sizeof(ps5_feature_config);
    case ReportId::ReportIdPs5GetResponse:
    case ReportId::ReportIdPs5GetAuthStatus:
        if (host_device)
        {
            return host_device->get_report(report_id, report_type, buffer, reqlen, nullptr);
        }
        return 0;
    }
    return 0;
}

uint8_t handle_player_leds_ps5(uint8_t player_mask)
{
    if (player_mask == 1)
    {
        return 1;
    }
    if (player_mask == 2)
    {
        return 2;
    }
    if (player_mask == 4)
    {
        return 3;
    }
    if (player_mask == 8)
    {
        return 4;
    }
    if (player_mask == 9)
    {
        return 5;
    }
    if (player_mask == 10)
    {
        return 6;
    }
    if (player_mask == 12)
    {
        return 7;
    }
    return 0;
}

void PS5GamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    std::shared_ptr<HidHost> host_device;
    auto auth_device = auth_devices.find(ModePs5);
    if (auth_device != auth_devices.end())
    {
        host_device = std::static_pointer_cast<HidHost>(auth_device->second);
    }
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
    {
        switch (report_id)
        {
        case 0:
            return;
        case ReportId::ReportIdPs5SetChallenge:
            if (host_device)
            {
                host_device->set_report(report_id, report_type, (uint8_t *)buffer, bufsize, nullptr);
            }
            return;
        }
        break;
    }
    case HID_REPORT_TYPE_OUTPUT:
    {
        ps5_output_report *report = (ps5_output_report *)buffer;
        if (report->light_bar_flag)
        {
            lightbar_red = report->lightbar_red;
            lightbar_green = report->lightbar_green;
            lightbar_blue = report->lightbar_blue;
        }
        if (report->vibration_flag)
        {
            rumble_left = report->motor_left;
            rumble_right = report->motor_right;
        }
        if (report->player_indicator_flag)
        {
            player_led = handle_player_leds_ps5(report->player_indicator);
        }

        break;
    }
    default:
        break;
    }
}