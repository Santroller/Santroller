#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include "main.hpp"
#include "config/config.hpp"
#include "managers/config_manager.hpp"
#include "managers/profile_manager.hpp"
#include "managers/device_manager.hpp"

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "config/config.hpp"
#include "devices/debug.hpp"
#include "common/tusb_types.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "emulation/usb/gh_arcade_device.h"
#include "emulation/usb/xinput_device.h"
#include "emulation/usb/xone_device.h"
#include "emulation/usb/ogxbox_device.h"
#include "emulation/usb/hid_device.h"
#include "emulation/usb/ps3_device.h"
#include "emulation/usb/ps4_device.h"
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
#include "pico/stdio/driver.h"
#include <pico/cyw43_arch.h>

#include "emulation/usb/usb_descriptors.h"
#include <pico_fota_bootloader/core.h>
#include "ring_buffer.h"
#include "hci.h"
#include "devices/bt/bluetooth_stack.hpp"

class HidConsoleBridge
{
public:
    static constexpr size_t max_packets_per_flush = 4;

    static HidConsoleBridge& instance()
    {
        static HidConsoleBridge bridge;
        return bridge;
    }

    void init()
    {
        ring_buffer_init(&m_buffer, m_buffer_storage, sizeof(m_buffer_storage), 0);
        m_initialized = true;
    }

    void enable()
    {
        stdio_set_driver_enabled(&m_driver, true);
    }

    void disable()
    {
        stdio_set_driver_enabled(&m_driver, false);
    }

    void write(const char *buf, int len)
    {
        if (!m_initialized)
        {
            return;
        }
        ring_buffer_push(&m_buffer, buf, len);
    }

    void flush()
    {
        auto& config_mgr = ConfigManager::instance();
        if (!m_initialized || !can_send() || config_mgr.is_reloading() || config_mgr.is_working())
        {
            return;
        }
        size_t packets_sent = 0;
        while (packets_sent < max_packets_per_flush && !ring_buffer_is_empty(&m_buffer) && can_send())
        {
            tu_memclr(m_event.event.console.data, sizeof(m_event.event.console.data));
            ring_buffer_pop(&m_buffer, m_event.event.console.data, sizeof(m_event.event.console.data) - 1);
            HIDConfigDevice::send_event(m_event, true);
            packets_sent++;
        }
    }

private:
    HidConsoleBridge() = default;

    static void out_chars(const char *buf, int len)
    {
        instance().write(buf, len);
    }

    static void out_flush()
    {
        instance().flush();
    }

    static int in_chars(char *buf, int len)
    {
        return 0;
    }

    static void set_chars_available_callback(void (*fn)(void *), void *param)
    {
    }

    static bool can_send()
    {
        return !HIDConfigDevice::tool_closed();
    }

    proto_Event m_event = {which_event : proto_Event_console_tag, event : {console : {data : {}}}};
    ring_buffer_t m_buffer;
    char m_buffer_storage[1024];
    bool m_initialized = false;
    stdio_driver_t m_driver = {
        .out_chars = out_chars,
        .out_flush = out_flush,
        .in_chars = in_chars,
        .set_chars_available_callback = set_chars_available_callback,
        .next = nullptr,
        .last_ended_with_cr = true,
        .crlf_enabled = true};
};

bool mode_recently_changed()
{
    return ConfigManager::instance().mode_recently_changed(millis());
}
void hid_task(void)
{
    auto& config_mgr = ConfigManager::instance();
    
    if (config_mgr.is_working())
    {
        return;
    }
    
    ConsoleMode requested_mode = config_mgr.get_requested_mode();
    ConsoleMode current_mode = config_mgr.get_current_mode();
    
    uint32_t now = millis();
    if (config_mgr.should_reinit(now))
    {
        if (!HIDConfigDevice::tool_closed())
        {
            proto_Event event = {which_event : proto_Event_reload_tag, event : {reload : {}}};
            HIDConfigDevice::send_event(event, true);
            tud_task();
        }
        printf("requested: %d current: %d init: %d\r\n", requested_mode, current_mode, config_mgr.get_reinit_time());
        config_mgr.begin_reinit();
        load();
        config_mgr.finish_reinit(millis());
        return;
    }
    update();
}

void send_debug(uint8_t *data, size_t len)
{
    proto_Event event = {which_event : proto_Event_debug_tag, event : {debug : {data : {size : (pb_size_t)len}}}};
    memcpy(event.event.debug.data.bytes, data, len);
    HIDConfigDevice::send_event(event, false);
}

static void initialize_device_stack()
{
    HIDConfigDevice::reset_keepalive();
    UsbDetectionState::instance().reset();
    const tusb_rhport_init_t rh_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL};
    tud_rhport_init(TUD_OPT_RHPORT, &rh_init);
    if (ConfigManager::instance().has_bluetooth())
    {
        BluetoothStack::instance().power_on();
        printf("bt init done\r\n");
    }
}

void reinitialize_device_stack()
{
    printf("Reinitializing device stack\r\n");
    if (ConfigManager::instance().has_bluetooth())
    {
        BluetoothStack::instance().power_off();
        printf("bt init done\r\n");
    }
    tud_deinit(TUD_OPT_RHPORT);
    initialize_device_stack();
}

bool send_timeout = false;

void update()
{
    DeviceManager::instance().update(false, false);
    ProfileManager::instance().update(false, false);
}

void initDebug()
{
    HidConsoleBridge::instance().enable();
}

void deinitDebug()
{
    HidConsoleBridge::instance().disable();
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
    ConfigManager::instance().sync_requested_mode_to_current();
    set_sys_clock_khz(180000, true);
    multicore_launch_core1(core1);
    adc_init();
    HidConsoleBridge::instance().init();
    EEPROM.start();
    if (!load())
    {
        // config was not valid, save a empty config
        load_empty();
        // load();
    }
    printf("init %d\r\n", ConfigManager::instance().get_current_mode());
    initialize_device_stack();
    ConfigManager::instance().finish_reinit(millis());
    
    while (1)
    {
        tud_task(); // tinyusb device task
        tuh_task();
        hid_task();
        HidConsoleBridge::instance().flush();
    }
    return 0;
}