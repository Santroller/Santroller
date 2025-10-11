#include "usb/device/ps3_device.h"
#include "usb/device/gamepad_device.h"
#include "protocols/ps4.hpp"
#include "enums.pb.h"
// Kick device into winusb mode on windows for RPCS3 compat
const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDsRPCS3 = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 1,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : 0,
            Reserved : 0x01,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};

uint8_t ef_byte = 0;
uint8_t master_bd_addr[6];
uint8_t f5_state = 0;
const uint8_t ps3_feature_01[] = {
    0x00, 0x01, 0x04, 0x00, 0x07, 0x0c, 0x01, 0x02,
    0x18, 0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11,
    0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t ps3_feature_f2[] = {
    0xf2, 0xff, 0xff, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // device bdaddr
    0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t ps3_feature_f5[] = {
    0x01, 0x00,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, // dummy PS3 bdaddr
    0xff, 0xf7, 0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t ps3_feature_f7[] = {
    0x01, 0x04, 0xc4, 0x02, 0xd6, 0x01, 0xee, 0xff,
    0x14, 0x13, 0x01, 0x02, 0xc4, 0x01, 0xd6, 0x00,
    0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x02, 0x62, 0x01, 0x02, 0x01,
    0x5e, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t ps3_feature_f8[] = {
    0x00, 0x01, 0x00, 0x00, 0x07, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Gyro and accel calibration are in here somewhere!
const uint8_t ps3_feature_ef[] = {
    0x00, 0xef, 0x04, 0x00, 0x05, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint16_t tud_hid_ps3_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    printf("get report: %02x %02x %04x\r\n", report_id, report_type, reqlen);
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
    case ReportId::ReportIdPs3F2:
        memcpy(buffer, ps3_feature_f2, sizeof(ps3_feature_f2));
        return sizeof(ps3_feature_f2);

    case ReportId::ReportIdPs3F8:
        memcpy(buffer, ps3_feature_f8, sizeof(ps3_feature_f8));
        buffer[7] = ef_byte;
        return sizeof(ps3_feature_f8);

    case ReportId::ReportIdPs3F7:
        memcpy(buffer, ps3_feature_f7, sizeof(ps3_feature_f7));
        return sizeof(ps3_feature_f7);
    case ReportId::ReportIdPs3F5:
        memcpy(buffer, ps3_feature_f5, sizeof(ps3_feature_f5));
        if (f5_state == 0)
        {
            /*
             * First request, tell that the bdaddr is not the one of the PS3.
             */
            f5_state = 1;
        }
        else
        {
            /*
             * Next requests, tell that the bdaddr is the one of the PS3.
             */
            memcpy(buffer + 2, master_bd_addr, sizeof(master_bd_addr));
        }
        return sizeof(ps3_feature_f5);
    case ReportId::ReportIdPs3EF:
        memcpy(buffer, ps3_feature_ef, sizeof(ps3_feature_ef));
        buffer[7] = ef_byte;
        return sizeof(ps3_feature_ef);
    case ReportId::ReportIdPs301:
        // PS3 sends this for a gamepad
        memcpy(buffer, ps3_feature_01, sizeof(ps3_feature_01));
        return sizeof(ps3_feature_01);
    }
    return 0;
}

void handle_player_leds_ds3(uint8_t player_mask)
{
    if (player_mask == 2)
    {
        tud_set_player_led_cb(1);
    }
    if (player_mask == 4)
    {
        tud_set_player_led_cb(2);
    }
    if (player_mask == 8)
    {
        tud_set_player_led_cb(3);
    }
    if (player_mask == 16)
    {
        tud_set_player_led_cb(4);
    }
    if (player_mask == 18)
    {
        tud_set_player_led_cb(5);
    }
    if (player_mask == 20)
    {
        tud_set_player_led_cb(6);
    }
    if (player_mask == 24)
    {
        tud_set_player_led_cb(7);
    }
}
void handle_player_leds_ps3(uint8_t player_mask)
{
    if (player_mask == 1)
    {
        tud_set_player_led_cb(1);
    }
    if (player_mask == 2)
    {
        tud_set_player_led_cb(2);
    }
    if (player_mask == 4)
    {
        tud_set_player_led_cb(3);
    }
    if (player_mask == 8)
    {
        tud_set_player_led_cb(4);
    }
    if (player_mask == 9)
    {
        tud_set_player_led_cb(5);
    }
    if (player_mask == 10)
    {
        tud_set_player_led_cb(6);
    }
    if (player_mask == 12)
    {
        tud_set_player_led_cb(7);
    }
}

void tud_hid_ps3_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    printf("set report: %02x %02x\r\n", report_id, report_type);
    (void)instance;
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
        switch (report_id)
        {
        case 0:

            return;
        case ReportId::ReportIdPs3EF:
            ef_byte = buffer[6];
        printf("%02x\r\n", ef_byte);
            return;
        }
    case HID_REPORT_TYPE_OUTPUT:
    {
        // rumble / led packets will be here

        uint8_t id = buffer[0];
        if (id == PS3_LED_ID)
        {
            handle_player_leds_ds3(buffer[9]);
            tud_set_rumble_cb(buffer[0x04], buffer[0x02] ? 0xff : 0);
        }
        else if (id == PS3_RUMBLE_ID)
        {
            // TODO: confirm this
            // instruments receive player leds over this report id with one format
            if (bufsize >= 8)
            {
                uint8_t player = buffer[3];
                handle_player_leds_ps3(player);
            }
            else
            {
                // and DS3s receive rumble, and the packet length is shorter
                tud_set_rumble_cb(buffer[0x05], buffer[0x03] ? 0xff : 0);
            }
        }
        else if (id == DJ_LED_ID)
        {
            uint8_t euphoria_on = buffer[2] * 0xFF;
            tud_set_euphoria_led_cb(euphoria_on);
        }
        break;
    }
    default:
        break;
    }
}

bool tud_hid_ps3_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR && request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR)
            {
                if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE)
                {
                    if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR)
                    {
                        tud_control_xfer(rhport, request, (void *)&DevCompatIDsRPCS3, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR));
                        return true;
                    }
                }
            }
        }
    }
    return false;
}