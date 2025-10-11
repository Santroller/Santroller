#include "usb/device/ps4_device.h"
#include "usb/device/gamepad_device.h"
#include "protocols/ps4.hpp"
#include "enums.pb.h"
#include "main.hpp"

static const int ps4_colors[4][3] = {
    {0x00, 0x00, 0x40}, /* Blue */
    {0x40, 0x00, 0x00}, /* Red */
    {0x00, 0x40, 0x00}, /* Green */
    {0x20, 0x00, 0x20}  /* Pink */
};
// TODO: set this correctly
#define PS4_TYPE 0
const uint8_t ps4_feature_config[] = {
    0x03, 0x21, 0x27, 0x04, 0x91, PS4_TYPE, 0x2c, 0x56,
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

uint16_t tud_hid_ps4_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
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
    case ReportId::ReportIdPs4Feature:
        seenPs4 = true;
        memcpy(buffer, ps4_feature_config, sizeof(ps4_feature_config));
        return sizeof(ps4_feature_config);
    case ReportId::ReportIdPs4GetResponse:
        // try to pass through to ps4 over usb host if one exists
        return 0;
    case ReportId::ReportIdPs4GetAuthStatus:
        // try to pass through to ps4 over usb host if one exists
    case ReportId::ReportIdPs4GetAuthPageSize:
        // try to pass through to ps4 over usb host if one exists
        // if it is a ds4 the request will fail but we need to return this one in that case
        memcpy(buffer, &ps4_pagesize_report, sizeof(ps4_pagesize_report));
        return sizeof(ps4_pagesize_report);
    }
    return 0;
}

void tud_hid_ps4_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
    {
        switch (report_id)
        {
        case 0:
            return;
        case ReportId::ReportIdPs4SetChallenge:
            // TODO: pass to ps4 controller for auth
            return;
        }
        break;
    }
    case HID_REPORT_TYPE_OUTPUT:
    {
        ps4_output_report *report = (ps4_output_report *)buffer;
        tud_set_lightbar_led_cb(report->lightbar_red, report->lightbar_green, report->lightbar_blue);
        tud_set_rumble_cb(report->motor_left, report->motor_right);
        for (int i = 0; i < 4; i++)
        {
            if (report->lightbar_red == ps4_colors[i][0] && report->lightbar_green == ps4_colors[i][1] && report->lightbar_blue == ps4_colors[i][2])
            {
                tud_set_player_led_cb(i + 1);
            }
        }
        break;
    }
    default:
        break;
    }
}