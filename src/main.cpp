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
#include "devices/debug.hpp"
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
#include <pico_fota_bootloader/core.h>

proto_Config config;
uint32_t timeSinceMode = millis();
bool seenPs4 = false;
bool seenWindowsXb1 = false;
bool seenOsDescriptorRead = false;
bool seenReadAnyDeviceString = false;
bool seenHidDescriptorRead = false;
bool reinit = false;
bool mode_recently_changed()
{
    return (millis() - timeSinceMode) < 2000;
}
void hid_task(void)
{
    if (working)
    {
        return;
    }
    if (newMode != mode || reinit)
    {
        printf("new: %d old: %d init: %d\r\n", newMode, mode, reinit);
        mode = newMode;
        reinit = false;
        seenPs4 = false;
        seenWindowsXb1 = false;
        seenOsDescriptorRead = false;
        seenReadAnyDeviceString = false;
        seenHidDescriptorRead = false;
        tud_deinit(BOARD_TUD_RHPORT);
        load(config);
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
        // TODO: do this right
        // else if (tud_ready() && (current_type == RockBandGuitar || current_type == RockBandDrums))
        // {
        //     // PS2 / Wii / WiiU does not read hid descriptor
        //     // The wii however will configure the usb device before it stops communicating
        //     newMode = ModeWiiRb;
        // }
        // else
        // {
        //     // But the PS2 does not. We also end up here on the wii/wiiu if a device does not have an explicit wii mode.
        //     newMode = ModePs3;
        // }
    }
    update();
}

void send_debug(uint8_t *data, size_t len)
{
    proto_Event event = {which_event : proto_Event_debug_tag, event : {debug : (pb_size_t)len}};
    memcpy(event.event.debug.data, data, len);
    HIDConfigDevice::send_event(event);
}

bool send_timeout = false;

void update()
{
    for (const auto &device : active_devices)
    {
        device->update(false, false);
    }
    for (const auto &instance : active_instances)
    {
        instance->process();
    }
    if (HIDConfigDevice::tool_closed())
    {

        for (const auto &profile : all_profiles)
        {
            for (auto &mapping : profile.second->triggers)
            {
                mapping->validate(false, false, false);
            }
        }
    }
}

void core1()
{
    multicore_lockout_victim_init();
    while (1)
    {
    }
}

int main()
{
    if (pfb_is_after_firmware_update())
    {
        // handle new firmare info if needed
    }
    if (pfb_is_after_rollback())
    {
        // handle performed rollback if needed
    }
    pfb_firmware_commit();
    newMode = mode;
    set_sys_clock_khz(120000, true);
    multicore_launch_core1(core1);
    adc_init();
    // stdio_uart_init_full(uart_get_instance(1), 115200, 8, 9);

    EEPROM.start();
    if (!load(config))
    {
        // config was not valid, save a empty config
        save_empty();
        load(config);
    }
    printf("init %d\r\n", mode);

    // init device stack on configured roothub port
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL};
    tud_rhport_init(BOARD_TUD_RHPORT, &rh_init);

    while (1)
    {
        tud_task(); // tinyusb device task
        tuh_task();
        hid_task();
    }
    return 0;
}