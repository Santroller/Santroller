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
// TODO: do we just throw bt on core1? did that work?
void core1()
{
    multicore_lockout_victim_init();
    while (1)
    {
    }
}
uint32_t timeSinceMode = millis();
bool seenPs4 = false;
bool seenWindowsXb1 = false;
bool seenOsDescriptorRead = false;
bool seenReadAnyDeviceString = false;
bool seenHidDescriptorRead = false;
bool reinit = false;
void hid_task(void)
{
    if (working)
    {
        return;
    }
    if (newMode != mode || reinit)
    {
        mode = newMode;
        reinit = false;
        seenPs4 = false;
        seenWindowsXb1 = false;
        seenOsDescriptorRead = false;
        seenReadAnyDeviceString = false;
        seenHidDescriptorRead = false;
        tud_deinit(BOARD_TUD_RHPORT);
        tud_init(BOARD_TUD_RHPORT);
        timeSinceMode = millis();
        return;
    }
    if ((millis() - timeSinceMode) > 2000 && mode == ModeHid && !seenWindowsXb1 && !seenOsDescriptorRead && !seenReadAnyDeviceString && tud_connected())
    {
        // Switch 2 does read the hid descriptor
        if (seenHidDescriptorRead)
        {
            newMode = ModeSwitch;
        }
        else if (tud_ready() && (current_type == RockBandGuitar || current_type == RockBandDrums))
        {
            // PS2 / Wii / WiiU does not read hid descriptor
            // The wii however will configure the usb device before it stops communicating
            newMode = ModeWiiRb;
        }
        else
        {
            // But the PS2 does not. We also end up here on the wii/wiiu if a device does not have an explicit wii mode.
            newMode = ModePs3;
        }
    }
    if ((millis() - timeSinceMode) > 2000 && mode == ModePs4 && !seenPs4)
    {
        newMode = ModePs3;
    }
    update(false);
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

static const uint8_t dpad_bindings[] = {0x08, 0x00, 0x04, 0x08, 0x06, 0x07, 0x05, 0x08, 0x02, 0x01, 0x03};
uint8_t gh5_mapping[] = {
    0x80, 0x15, 0x4D, 0x30, 0x9A, 0x99, 0x66,
    0x65, 0xC9, 0xC7, 0xC8, 0xC6, 0xAF, 0xAD,
    0xAE, 0xAC, 0xFF, 0xFB, 0xFD, 0xF9, 0xFE,
    0xFA, 0xFC, 0xF8, 0xE6, 0xE2, 0xE4, 0xE0,
    0xE5, 0xE1, 0xE3, 0xDF};
void update(bool full_poll)
{
    uint8_t out[64] = {0};
    for (const auto &device : devices)
    {
        device.second->update(full_poll);
    }
    // // If we are configuring, disable triggers
    for (const auto &trigger : triggers)
    {
        trigger->update(tool_closed());
    }
    switch (mode)
    {
    case ModeHid:
        if (tud_hid_ready())
        {
            for (const auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_hid(out);
            }

            // convert bitmask dpad to actual hid dpad
            PCGamepad_Data_t *report = (PCGamepad_Data_t *)out;
            report->dpad = dpad_bindings[report->dpad];
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                PCGuitarHeroGuitar_Data_t *reportGh = (PCGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = gh5_mapping[reportGh->slider];
            }

            tud_hid_report(ReportId::ReportIdGamepad, out, sizeof(PCGamepad_Data_t));
        }
        break;
    case ModeSwitch:
    case ModeWiiRb:
    case ModePs3:
        if (tud_hid_ready())
        {
            if (current_type == Gamepad && mode == ModePs3)
            {
                PS3Gamepad_Data_t *report = (PS3Gamepad_Data_t *)out;
                report->accelX = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->accelY = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->accelZ = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->gyro = __builtin_bswap16(PS3_ACCEL_CENTER);
                report->leftStickX = PS3_STICK_CENTER;
                report->leftStickY = PS3_STICK_CENTER;
                report->rightStickX = PS3_STICK_CENTER;
                report->rightStickY = PS3_STICK_CENTER;
            }
            else
            {
                PS3Dpad_Data_t *gamepad = (PS3Dpad_Data_t *)out;
                gamepad->accelX = PS3_ACCEL_CENTER;
                gamepad->accelY = PS3_ACCEL_CENTER;
                gamepad->accelZ = PS3_ACCEL_CENTER;
                gamepad->gyro = PS3_ACCEL_CENTER;
                gamepad->leftStickX = PS3_STICK_CENTER;
                gamepad->leftStickY = PS3_STICK_CENTER;
                if (current_type == ProKeys)
                {
                    gamepad->rightStickX = 0;
                    gamepad->rightStickY = 0;
                }
                else
                {
                    gamepad->rightStickX = PS3_STICK_CENTER;
                    gamepad->rightStickY = PS3_STICK_CENTER;
                }
            }
            for (const auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_ps3(out);
            }
            if (current_type == Gamepad && mode == ModePs3)
            {
                // Gamepads use a totally different report format
                tud_hid_report(ReportId::ReportIdGamepad, &out, sizeof(PS3Gamepad_Data_t));
                break;
            }
            // convert bitmask dpad to actual hid dpad
            PS3Dpad_Data_t *report = (PS3Dpad_Data_t *)out;
            report->dpad = dpad_bindings[report->dpad];
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                PCGuitarHeroGuitar_Data_t *reportGh = (PCGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = gh5_mapping[reportGh->slider];
            }

            tud_hid_report(0, out, sizeof(PS3Dpad_Data_t));
        }
        break;
    case ModePs4:
        if (tud_hid_ready())
        {
            for (const auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_ps4(out);
            }
            PS4Dpad_Data_t *gamepad = (PS4Dpad_Data_t *)out;
            gamepad->leftStickX = PS3_STICK_CENTER;
            gamepad->leftStickY = PS3_STICK_CENTER;
            gamepad->rightStickX = PS3_STICK_CENTER;
            gamepad->rightStickY = PS3_STICK_CENTER;
            // convert bitmask dpad to actual hid dpad
            gamepad->dpad = dpad_bindings[gamepad->dpad];

            tud_hid_report(ReportId::ReportIdGamepad, out, sizeof(PS4Dpad_Data_t));
        }
        break;
    case ModeOgXbox:
        if (tud_ogxbox_n_ready(0))
        {
            OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)out;
            report->rid = 0;
            report->rsize = sizeof(OGXboxGamepad_Data_t);
            for (const auto &mapping : mappings)
            {
                mapping->update(full_poll);
                mapping->update_ogxbox(out);
            }
            if (current_type == GuitarHeroGuitar)
            {
                // convert bitmask slider to actual hid slider
                OGXboxGuitarHeroGuitar_Data_t *reportGh = (OGXboxGuitarHeroGuitar_Data_t *)out;
                reportGh->slider = -((int8_t)((gh5_mapping[reportGh->slider]) ^ 0x80) * -257);
            }

            tud_ogxbox_n_report(0, out, sizeof(XInputGamepad_Data_t));
        }
        break;
    case ModeXbox360:
    {
        for (int i = 0; i < 4; i++)
        {
            if (tud_xinput_n_ready(i))
            {
                XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)out;
                report->rid = 0;
                report->rsize = sizeof(XInputGamepad_Data_t);
                for (const auto &mapping : mappings)
                {
                    mapping->update(full_poll);
                    mapping->update_xinput(out);
                }
                if (current_type == GuitarHeroGuitar)
                {
                    // convert bitmask slider to actual hid slider
                    XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)out;
                    reportGh->slider = -((int8_t)((gh5_mapping[reportGh->slider]) ^ 0x80) * -257);
                }

                tud_xinput_n_report(i, out, sizeof(XInputGamepad_Data_t));
            }
        }
        break;
    }
    case ModeXboxOne:
        break;
    }
}

int main()
{
    newMode = mode;
    set_sys_clock_khz(120000, true);
    multicore_launch_core1(core1);
    adc_init();
    EEPROM.start();

    proto_Config config;
    if (!load(config))
    {
        // config was not valid, save a empty config
        save(&config);
    }
    printf("init %d\r\n", mode);

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    while (1)
    {
        tud_task(); // tinyusb device task
        tuh_task();
        hid_task();
    }
    return 0;
}