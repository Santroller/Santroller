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
#include "managers/inactivity_manager.hpp"

#include "config.pb.h"
#include "pico/stdlib.h"
#include "config/FlashPROM.h"
#include "CRC32.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
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
#include "secondary_pico.hpp"
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
        ring_buffer_push_unsafe(&m_buffer, buf, len);
    }

    void flush()
    {
        auto& config_mgr = ConfigManager::instance();
        if (!m_initialized || !can_send() || config_mgr.is_reloading() || config_mgr.is_working())
        {
            return;
        }
        size_t packets_sent = 0;
        // Only queue into free slots; never wait on the PC, as that stalls tuh_task().
        while (packets_sent < max_packets_per_flush && !ring_buffer_is_empty_unsafe(&m_buffer) && HIDConfigDevice::has_event_space())
        {
            tu_memclr(m_event.event.console.data, sizeof(m_event.event.console.data));
            ring_buffer_pop_unsafe(&m_buffer, m_event.event.console.data, sizeof(m_event.event.console.data) - 1);
            HIDConfigDevice::send_event(m_event, false);
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

// Config chunks arrive back to back, so a gap this long means the tool isn't going to finish
static constexpr uint32_t CONFIG_WRITE_STALL_MS = 2000;

// Kept out of reload_config, so the event isn't on the stack while the config loads
static void __noinline send_reload_event()
{
    proto_Event event = {which_event : proto_Event_reload_tag, event : {reload : {}}};
    HIDConfigDevice::send_event(event, true);
    // The reload may reinit the device stack, which would discard anything still queued
    HIDConfigDevice::flush_events(100000);
}

// Reloads the config once it has changed, returning true if it did. This is called straight from
// the main loop with as little as possible on the stack, as decoding deeply nested inputs (eg a
// toggle of a shortcut) recurses through nanopb and needs most of core 0's stack.
static bool __noinline reload_config()
{
    auto& config_mgr = ConfigManager::instance();
    if (config_mgr.is_working() || !config_mgr.should_reinit(millis()))
    {
        return false;
    }
    if (!HIDConfigDevice::tool_closed())
    {
        send_reload_event();
    }
    config_mgr.begin_reinit();
    load();
    config_mgr.finish_reinit(millis());
    return true;
}

void hid_task(void)
{
    auto& config_mgr = ConfigManager::instance();
    
    uint32_t now = millis();
    if (config_mgr.is_working())
    {
        // The tool went away partway through writing a config (closed, crashed, unplugged from the PC)
        if (config_mgr.working_stalled(now, CONFIG_WRITE_STALL_MS))
        {
            printf("config write stalled, abandoning it\r\n");
            abort_config_write();
        }
        return;
    }
    if (config_mgr.take_device_stack_restart())
    {
        printf("restarting device stack\r\n");
        reinitialize_device_stack();
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
    ProfileManager::instance().initialize_device_bluetooth();
}

void reinitialize_device_stack()
{
    ProfileManager::instance().deinitialize_device_bluetooth();
    tud_deinit(TUD_OPT_RHPORT);
    initialize_device_stack();
}

bool send_timeout = false;

void update()
{
    DeviceManager::instance().update(false, false);
    ProfileManager::instance().update(false, false);
    InactivityManager::instance().tick();
}

void initDebug()
{
    HidConsoleBridge::instance().enable();
}

void deinitDebug()
{
    HidConsoleBridge::instance().disable();
}


// Core 1's stack is in main RAM rather than SCRATCH_X, so core 0's stack can use both scratch banks
static constexpr size_t CORE1_STACK_SIZE = 0x1000;
static uint32_t core1_stack[CORE1_STACK_SIZE / sizeof(uint32_t)];

void core1()
{
    // Our own flash writes keep core1 running (see flash_write_begin), but the SDK's flash_safe_execute
    // (used by btstack's flash bank via cyw43_arch_init) fails, and asserts, unless core1 can be locked out
    multicore_lockout_victim_init();
    flash_core1_loop();
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
    // 1.15V is what the SDK uses for its 200MHz RP2040 preset, so this keeps us inside the rated range
    vreg_set_voltage(VREG_VOLTAGE_1_15);
    // Let the regulator settle before raising the clock, like the SDK does
    busy_wait_us(1000);
    // A multiple of 96MHz, so PIO-USB's 48MHz TX and 96MHz RX clocks divide evenly. A fractional divider
    // jitters every bit edge, which some devices (like the 360 wireless receiver) seem sensitive to
    set_sys_clock_khz(192000, true);
    multicore_launch_core1_with_stack(core1, core1_stack, sizeof(core1_stack));
    adc_init();
    HidConsoleBridge::instance().init();
    EEPROM.start();
    if (!load())
    {
        // config was not valid, save a empty config
        load_empty();
        // load();
    }
    if (is_secondary_pico_mode())
    {
        printf("Entering secondary pico coprocessor mode\r\n");
        secondary_pico_coprocessor_loop();
    }
    printf("init %d\r\n", ConfigManager::instance().get_current_mode());
    initialize_device_stack();
    ConfigManager::instance().finish_reinit(millis());

    while (1)
    {
        tud_task(); // tinyusb device task
        tuh_task();
        if (!reload_config())
        {
            hid_task();
        }
        HidConsoleBridge::instance().flush();
        EEPROM.tick();
        BluetoothStack::instance().tick();
    }
    return 0;
}