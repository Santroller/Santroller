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
    update(false);
}

void send_debug(uint8_t *data, size_t len)
{
    proto_Event event = {which_event : proto_Event_debug_tag, event : {debug : (pb_size_t)len}};
    memcpy(event.event.debug.data, data, len);
    HIDConfigDevice::send_event(event);
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

void update(bool full_poll)
{
    for (const auto &device : devices)
    {
        device.second->update(full_poll);
    }
    for (const auto &instance : active_instances)
    {
        instance->process(full_poll);
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
        first_load();
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