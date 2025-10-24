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
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "config.hpp"
#include "common/tusb_types.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "usb/device/gh_arcade_device.h"
#include "usb/device/xinput_device.h"
#include "usb/device/xone_device.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/hid_device.h"
#include "usb/device/ps3_device.h"
#include "usb/device/ps4_device.h"
#include "uart.hpp"
#include "protocols/hid.hpp"
#include "main.hpp"
#include "utils.h"
#include "hardware/uart.h"
#include "pico/bootrom.h"
#include "tusb.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "common/tusb_types.h"
#include "pio_usb.h"
#include "usb/device/hid_driver.h"

#include "usb/usb_descriptors.h"
#include "console_mode.h"
hidd_driver_t *hid_driver = NULL;

hidd_driver_t hid_drivers[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "PS3_Device",
#endif
        .mode = ConsoleMode::Ps3,
        .open = NULL,
        .control_xfer_cb = tud_hid_ps3_control_xfer_cb,
        .get_report_cb = tud_hid_ps3_get_report_cb,
        .set_report_cb = tud_hid_ps3_set_report_cb},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "PS4_Device",
#endif
        .mode = ConsoleMode::Ps4,
        .open = NULL,
        .control_xfer_cb = NULL,
        .get_report_cb = tud_hid_ps4_get_report_cb,
        .set_report_cb = tud_hid_ps4_set_report_cb},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "Generic_Device",
#endif
        .mode = ConsoleMode::Hid,
        .open = NULL,
        .control_xfer_cb = tud_hid_generic_control_xfer_cb,
        .get_report_cb = tud_hid_generic_get_report_cb,
        .set_report_cb = tud_hid_generic_set_report_cb},
};
usbd_class_driver_t driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "Hid_Device",
#endif
        .init = hidd_init,
        .reset = hidd_reset,
        .open = hidd_open,
        .control_xfer_cb = hidd_control_xfer_cb,
        .xfer_cb = hidd_xfer_cb,
        .sof = NULL},
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
// TODO: do we just throw bt on core1? did that work?
void core1()
{
    multicore_lockout_victim_init();
    while (1)
    {
    }
}
uint32_t timeSinceMode = 0;
bool seenPs4 = false;
bool seenWindowsXb1 = false;
bool seenOsDescriptorRead = false;
bool seenReadAnyDeviceString = false;
bool seenHidDescriptorRead = false;
bool reinit = false;
void hid_task(void)
{
    if (newMode != mode || reinit)
    {
        mode = newMode;
        // TODO: we technically dont _have_ to reset all of these
        reinit = false;
        seenPs4 = false;
        seenWindowsXb1 = false;
        seenOsDescriptorRead = false;
        seenReadAnyDeviceString = false;
        seenHidDescriptorRead = false;
        tud_deinit(BOARD_TUD_RHPORT);
        tud_init(BOARD_TUD_RHPORT);

        for (auto &driver : hid_drivers)
        {
            if (driver.mode == mode)
            {
                hid_driver = &driver;
            }
        }
        timeSinceMode = millis();
    }
    if ((millis() - timeSinceMode) > 2000 && mode == ConsoleMode::Hid && !seenWindowsXb1 && !seenOsDescriptorRead && !seenReadAnyDeviceString && tud_connected())
    {
        // Switch 2 does read the hid descriptor
        if (seenHidDescriptorRead)
        {
            newMode = ConsoleMode::Switch;
        }
        else if (tud_ready() && (current_type == RockBandGuitar || current_type == RockBandDrums))
        {
            // PS2 / Wii / WiiU does not read hid descriptor
            // The wii however will configure the usb device before it stops communicating
            newMode = ConsoleMode::WiiRb;
        }
        else
        {
            // But the PS2 does not. We also end up here on the wii/wiiu if a device does not have an explicit wii mode.
            newMode = ConsoleMode::Ps3;
        }
    }
    if ((millis() - timeSinceMode) > 2000 && mode == ConsoleMode::Ps4 && !seenPs4)
    {
        newMode = ConsoleMode::Ps3;
    }
    update(false);
}
uint32_t lastKeepAlive = 0;

bool tool_closed()
{
    return millis() - lastKeepAlive > 500;
}

void send_event(proto_Event event)
{
    // Haven't received data from the tool recently so don't send out events for it
    if (tool_closed() || !tud_ready())
    {
        return;
    }
    // Make sure events are always sent out
    while (!tud_hid_ready())
    {
        if (tool_closed() || !tud_ready())
        {
            return;
        }
        tud_task();
    }
    uint8_t buffer[63];
    pb_ostream_t outputStream = pb_ostream_from_buffer(buffer, 63);
    pb_encode(&outputStream, proto_Event_fields, &event);
    tud_hid_report(ReportId::ReportIdConfig, buffer, outputStream.bytes_written);
}

void send_debug(uint8_t *data, size_t len)
{
    proto_Event event = {which_event : proto_Event_debug_tag, event : {debug : (pb_size_t)len}};
    memcpy(event.event.debug.data, data, len);
    send_event(event);
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
bool send_timeout = false;
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report, uint16_t len)
{
    printf("report! %d\r\n", len);
    if (report[0] == 0x81 && report[1] == 0x01)
    {
        uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
        tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
        send_timeout = false;
    }
    else if (!send_timeout && report[0] == 0x81 && report[1] == 0x02)
    {
        send_timeout = true;
        uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x03 /* PROCON_USB_ENABLE */};
        tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    }
    else if (report[0] == 0x81 && report[1] == 0x03)
    {
        uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
        tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    }
    else if (report[0] == 0x81 && report[1] == 0x02)
    {
        uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x04 /* PROCON_USB_ENABLE */};
        tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    }
    if (report[0] == 0x30)
    {
        for (int i = 0; i < len; i++)
        {
            printf("%02x, ", report[i]);
        }
        printf("\r\n");
    }
    tuh_hid_receive_report(dev_addr, idx);
}
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report_desc, uint16_t desc_len)
{

    printf("mount! %d\r\n", desc_len);
    tuh_hid_receive_report(dev_addr, idx);
}

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = TU_ARRAY_SIZE(driver);
    return driver;
}
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // printf("control req %02x %02x %02x %02x %04x %04x\r\n", request->bmRequestType_bit.direction,request->bmRequestType_bit.type,request->bmRequestType_bit.recipient, request->bRequest, request->wIndex, request->wValue);
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
    for (auto &d : driver)
    {
        if (d.control_xfer_cb(rhport, stage, request))
        {
            return true;
        }
    }
    if (hid_driver && hid_driver->control_xfer_cb)
    {
        return hid_driver->control_xfer_cb(rhport, stage, request);
    }
    return false;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
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
            buffer[0] = report_id;
            buffer++;
            uint32_t ret = copy_config(buffer, start);
            start += ret;
            return ret + 1;
        }
        case ReportId::ReportIdConfigInfo:
            buffer[0] = report_id;
            buffer++;
            start = 0;
            return copy_config_info(buffer) + 1;
        }
        break;
    case HID_REPORT_TYPE_INPUT:
        // TODO: return the relevant inputs here
        return 1;
    default:
        break;
    }
    if (hid_driver)
    {
        return hid_driver->get_report_cb(instance, report_id, report_type, buffer, reqlen);
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    if (hid_driver)
    {
        hid_driver->set_report_cb(instance, report_id, report_type, buffer, bufsize);
    }
    if (report_type == HID_REPORT_TYPE_FEATURE)
    {
        // skip over report id
        buffer++;
        bufsize--;
        switch (report_id)
        {
        case ReportId::ReportIdConfig:
            lastKeepAlive = millis();
            write_config(buffer, bufsize, start);
            start += bufsize;
            break;
        case ReportId::ReportIdConfigInfo:
            lastKeepAlive = millis();
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
    }
}

static bool clearedIn = false;
static bool clearedOut = false;

void tud_hid_clear_feature_cb(uint8_t instance, uint8_t endpoint)
{
    (void)instance;
    (void)endpoint;
    clearedIn |= tu_edpt_dir(endpoint) == TUSB_DIR_IN;
    clearedOut |= tu_edpt_dir(endpoint) == TUSB_DIR_OUT;
    if (clearedIn && clearedOut)
    {
        newMode = ConsoleMode::Switch;
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

void tud_detected_console(ConsoleMode mode)
{
}


int main()
{
    newMode = mode;
    set_sys_clock_khz(120000, true);
    multicore_launch_core1(core1);
    stdio_init_all();
    printf("init %d\r\n", mode);
    adc_init();
    EEPROM.start();

    proto_Config config;
    if (!load(config))
    {
        // config was not valid, save a empty config
        save(&config);
    }

    for (auto &driver : hid_drivers)
    {
        if (driver.mode == mode)
        {
            hid_driver = &driver;
        }
    }

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);
    
    while (1)
    {
        hid_task();
        tud_task(); // tinyusb device task
        tuh_task();
    }
    return 0;
}