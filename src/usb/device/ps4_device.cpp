#include "usb/device/ps4_device.h"
#include "protocols/ps4.hpp"
#include "enums.pb.h"
#include "main.hpp"
#include "config.hpp"
#include "hid_reports.h"
#include "usb/usb_devices.h"
#include "usb/host/hid_host.h"

static const int ps4_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};
uint8_t ps4_feature_config[] = {
    0x03, 0x21, 0x27, 0x04, 0x91, /*type*/ 0x00, 0x2c, 0x56,
    0xa0, 0x0f, 0x3d, 0x00, 0x00, 0x04, 0x01, 0x00,
    0x00, 0x20, 0x0d, 0x0d, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

AuthPageSizeReport ps4_pagesize_report = {
    type : 0xF3,
    u1 : 0x00,
    size_challenge : 0x38,
    size_response : 0x38,
    u4 : {0x00, 0x00, 0x00, 0x00}
};

uint8_t const desc_hid_report_ps4[] =
    {
        TUD_HID_REPORT_DESC_PS4_THIRDPARTY_GAMEPAD(HID_REPORT_ID(ReportIdGamepad))};

PS4GamepadDevice::PS4GamepadDevice()
{
}
void PS4GamepadDevice::initialize()
{
    m_epin = next_epin();
    m_epout = next_epout();
    usb_instances_by_epin[m_epin & (~0x80)] = usb_instances[interface_id];
    usb_instances_by_epout[m_epout] = usb_instances[interface_id];

    PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)initialReport;
    gamepad->report_id = 1;
    gamepad->leftStickX = PS3_STICK_CENTER;
    gamepad->leftStickY = PS3_STICK_CENTER;
    gamepad->rightStickX = PS3_STICK_CENTER;
    gamepad->rightStickY = PS3_STICK_CENTER;
    gamepad->reportCounter = 1;
}
void PS4GamepadDevice::process()
{
    if (!ready())
        return;
    memcpy(epin_buf, &initialReport, sizeof(initialReport));
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
            mapping->update_ps4(epin_buf);
        }
        for (const auto &led : profile->leds)
        {
            led->update(false, false);
        }
    }
    PS5Dpad_Data_t *gamepad = (PS5Dpad_Data_t *)epin_buf;
    // convert bitmask dpad to actual hid dpad
    gamepad->dpad = GamepadButtonMapping::dpad_bindings[gamepad->dpad];
    send_report(sizeof(PS4Dpad_Data_t), 0, epin_buf);
    PS4Dpad_Data_t *initial = (PS4Dpad_Data_t *)initialReport;
    initial->reportCounter++;
    if (initial->reportCounter == 0) {
        initial->reportCounter = 1;
    }
}

size_t PS4GamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t PS4GamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps4), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

size_t PS4GamepadDevice::device_name(uint8_t idx, char *desc) 
{
    return 0;
}

void PS4GamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
}
const uint8_t *PS4GamepadDevice::report_descriptor()
{
    return desc_hid_report_ps4;
}

uint16_t PS4GamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_ps4);
}

uint16_t PS4GamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    if (report_type != HID_REPORT_TYPE_FEATURE)
    {
        return 0;
    }
    bool status;
    std::shared_ptr<HidHost> host_device = nullptr;
    auto auth_device = auth_devices.find(ModePs4);
    if (auth_device != auth_devices.end())
    {
        host_device = std::static_pointer_cast<HidHost>(auth_device->second);
    }

    switch (report_id)
    {
    case ReportId::ReportIdPs4Feature:
        memcpy(buffer, ps4_feature_config, sizeof(ps4_feature_config));
        switch (subtype)
        {
        case Gamepad:
            buffer[5] = PS4_GAMEPAD;
            break;
        case RockBandGuitar:
            buffer[5] = PS4_GUITAR;
            buffer[24] = 0x07;
            break;
        case GuitarHeroGuitar:
        case LiveGuitar:
            buffer[5] = PS4_GUITAR;
            buffer[24] = 0x06;
            break;
        case GuitarHeroDrums:
            buffer[5] = PS4_DRUMS;
            buffer[24] = 0x1f;
            break;
        case RockBandDrums:
            buffer[5] = PS4_DRUMS;
            buffer[24] = 0x1f;
            break;
        case FightStick:
            buffer[5] = PS4_FIGHTSTICK;
            break;
        default:
            break;
        }
        return sizeof(ps4_feature_config);
    case ReportId::ReportIdPs4GetResponse:
    case ReportId::ReportIdPs4GetAuthStatus:
        if (host_device != nullptr)
        {
            return host_device->get_report(report_id, report_type, buffer, reqlen, nullptr);
        }
        return 0;
    case ReportId::ReportIdPs4GetAuthPageSize:

        if (host_device != nullptr)
        {
            status = false;
            reqlen = host_device->get_report(report_id, report_type, buffer, reqlen, &status);
            if (status)
            {
                return reqlen;
            }
        }
        // try to pass through to ps4 over usb host if one exists
        // if it is a ds4 the request will fail but we need to return this one in that case
        memcpy(buffer, &ps4_pagesize_report, sizeof(ps4_pagesize_report));
        return sizeof(ps4_pagesize_report);
    }
    return 0;
}

void PS4GamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    std::shared_ptr<HidHost> host_device = nullptr;
    auto auth_device = auth_devices.find(ModePs4);
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
        case ReportId::ReportIdPs4SetChallenge:
            if (host_device != nullptr)
            {
                host_device->set_report(report_id, report_type, (uint8_t *)buffer, bufsize, nullptr);
            }
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
            if (report->lightbar_red == ps4_colors[i][0] && report->lightbar_green == ps4_colors[i][1] && report->lightbar_blue == ps4_colors[i][2])
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