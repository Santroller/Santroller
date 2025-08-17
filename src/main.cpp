#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <stdint.h>
#include "config.hpp"
#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "config.hpp"
#include "common/tusb_types.h"
#include "device/usbd_pvt.h"
#include "usb/device/gh_arcade_device.h"
#include "usb/device/xinput_device.h"
#include "usb/device/xone_device.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/ps_device.h"
#include "protocols/hid.hpp"
#include "main.hpp"
#include "utils.h"
#include "hardware/uart.h"
#include "pico/bootrom.h"
#include "tusb.h"

#include "usb/usb_descriptors.h"
#include "console_mode.h"
void core1()
{
    multicore_lockout_victim_init();
    while (1)
    {
    }
}
void hid_task(void)
{
    if (!tud_hid_ready())
        return;
    update(false);
    PCGamepad_Data_t out = {0};

    tud_hid_report(ReportId::ReportIdGamepad, &out, sizeof(out));
}
long lastKeepAlive = 0;

void send_event(proto_Event event)
{
    // Haven't received data from the tool recently so don't send out events for it
    if (millis() - lastKeepAlive > 20) {
        return;
    }
    // Make sure events are always sent out
    while (!tud_hid_ready() && tud_ready())
    {
        tud_task();
    }
    uint8_t buffer[63];
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 63);
    pb_encode(&outputStream, proto_Event_fields, &event);
    tud_hid_report(ReportId::ReportIdConfig, buffer, outputStream.bytes_written);
}

void send_debug(uint8_t *data, size_t len)
{
    proto_Event event = {which_event : proto_Event_debug_tag, event : {debug : len}};
    memcpy(event.event.debug.data, data, len);
    send_event(event);
}

ConsoleMode mode = ConsoleMode::Hid;
int main()
{
    multicore_launch_core1(core1);
    stdio_init_all();

    adc_init();
    EEPROM.start();

    proto_Config config;
    if (!load(config))
    {
        // config was not valid, save a empty config
        save(&config);
    }

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    while (1)
    {
        tud_task(); // tinyusb device task

        hid_task();
    }
    return 0;
}

const OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR ExtendedIDs = {
    TotalLength : sizeof(OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR),
    Version : 0x0100,
    Index : DESC_EXTENDED_PROPERTIES_DESCRIPTOR,
    TotalSections : 1,
    SectionSize : 132,
    ExtendedID : {
        PropertyDataType : 1,
        PropertyNameLength : 40,
        PropertyName : {'D', 'e', 'v', 'i', 'c', 'e', 'I', 'n', 't', 'e',
                        'r', 'f', 'a', 'c', 'e', 'G', 'U', 'I', 'D', '\0'},
        PropertyDataLength : 78,
        PropertyData :
            {'{', 'D', 'F', '5', '9', '0', '3', '7', 'D', '-', '7', 'C', '9',
             '2', '-', '4', '1', '5', '5', '-', 'A', 'C', '1', '2', '-', '7',
             'D', '7', '0', '0', 'A', '3', '1', '3', 'D', '7', '9', '}', '\0'}
    }
};

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    (void)len;
}
uint32_t start = 0;

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    printf("get: %d %d\r\n", report_type, report_id);
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
        switch (report_id)
        {
        case ReportId::ReportIdConfig:
        {
            uint32_t ret = copy_config(buffer, start);
            start += ret;
            return ret;
        }
        case ReportId::ReportIdConfigInfo:
            start = 0;
            return copy_config_info(buffer);
        }
    case HID_REPORT_TYPE_INPUT:
        // TODO: return the relevant inputs here
        return 0;
    default:
        break;
    }
    if (mode == ConsoleMode::Ps3 || mode == ConsoleMode::Ps4)
    {
        return tud_hid_ps_get_report_cb(instance, mode, report_id, report_type, buffer, reqlen);
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    printf("set: %d %d\r\n", report_type, report_id);
    if (mode == ConsoleMode::Ps3 || mode == ConsoleMode::Ps4)
    {
        tud_hid_ps_set_report_cb(instance, mode, report_id, report_type, buffer, bufsize);
    }
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
        switch (report_id)
        {
        case ReportId::ReportIdConfig:
            write_config(buffer, bufsize, start);
            start += bufsize;
            break;
        case ReportId::ReportIdConfigInfo:
            start = 0;
            write_config_info(buffer, bufsize);
            break;
        case ReportId::ReportIdLoaded:
            lastKeepAlive = millis();
            update(true);
            break;
        case ReportId::ReportIdKeepalive:
            lastKeepAlive = millis();
            break;
        case ReportId::ReportIdBootloader:
            reset_usb_boot(0, 0);
            break;
        }
    case HID_REPORT_TYPE_OUTPUT:
    // TODO: handle vibration and led reports and things like that for just pc hid
    default:
        break;
    }
}
void tud_set_rumble_cb(uint8_t left, uint8_t right)
{
}
void tud_set_player_led_cb(uint8_t player)
{
}
void tud_set_lightbar_led_cb(uint8_t red, uint8_t green, uint8_t blue)
{
}
void tud_set_euphoria_led_cb(uint8_t led)
{
}
void tud_set_stage_kit_cb(uint8_t command, uint8_t param)
{
}
void tud_gh_arcade_set_side_cb(uint8_t instance, uint8_t side)
{
    (void)instance;
    (void)side;
}
void tud_xone_set_report_cb(uint8_t instance, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
}
void tud_ogxbox_set_report_cb(uint8_t instance, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
}

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
            {
                if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
                {
                    if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR)
                    {
                        tud_control_xfer(rhport, request, (void *)&ExtendedIDs, ExtendedIDs.TotalLength);
                        return true;
                    }
                }
            }
        }
    }
    if (xinputd_control_xfer_cb(rhport, stage, request))
    {
        return true;
    }
    if (xoned_control_xfer_cb(rhport, stage, request))
    {
        return true;
    }
    if (ogxboxd_control_xfer_cb(rhport, stage, request))
    {
        return true;
    }
    if (gh_arcaded_control_xfer_cb(rhport, stage, request))
    {
        return true;
    }
    if (mode == ConsoleMode::Ps3 || mode == ConsoleMode::Ps4)
    {
        return tud_hid_ps_control_xfer_cb(rhport, mode, stage, request);
    }
    return false;
}

usbd_class_driver_t driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Device",
#endif
        .init = xinputd_init,
        .reset = xinputd_reset,
        .open = xinputd_open,
        .control_xfer_cb = xinputd_control_xfer_cb,
        .xfer_cb = xinputd_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XOne_Device",
#endif
        .init = xoned_init,
        .reset = xoned_reset,
        .open = xoned_open,
        .control_xfer_cb = xoned_control_xfer_cb,
        .xfer_cb = xoned_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "OgXbox_Device",
#endif
        .init = ogxboxd_init,
        .reset = ogxboxd_reset,
        .open = ogxboxd_open,
        .control_xfer_cb = ogxboxd_control_xfer_cb,
        .xfer_cb = ogxboxd_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "GhArcade_Device",
#endif
        .init = gh_arcaded_init,
        .reset = gh_arcaded_reset,
        .open = gh_arcaded_open,
        .control_xfer_cb = gh_arcaded_control_xfer_cb,
        .xfer_cb = gh_arcaded_xfer_cb,
        .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = TU_ARRAY_SIZE(driver);
    return driver;
}