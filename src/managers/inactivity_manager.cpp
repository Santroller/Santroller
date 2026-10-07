#include "managers/inactivity_manager.hpp"
#include "managers/device_manager.hpp"
#include "managers/profile_manager.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "emulation/usb/hid_device.h"
#include "leds/leds.hpp"
#include "utils.h"
#include "tusb.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/sleep.h"
#include <pico/cyw43_arch.h>

void InactivityManager::configure(const proto_InactivityConfig *config)
{
    m_sleep_timeout_ms = 0;
    m_led_timeout_ms = 0;
    m_wake_pin = -1;
    m_wake_active_high = false;
    if (config)
    {
        // sleeping with no way to wake up would need a power cycle, so sleep needs a wake pin
        if (config->has_wakePin && config->wakePin >= 0)
        {
            m_wake_pin = config->wakePin;
            m_wake_active_high = config->has_wakeActiveHigh && config->wakeActiveHigh;
            m_sleep_timeout_ms = config->has_sleepTimeoutSec ? config->sleepTimeoutSec * 1000 : 0;
        }
        m_led_timeout_ms = config->has_ledTimeoutSec ? config->ledTimeoutSec * 1000 : 0;
    }
    // A config change counts as activity, so new timeouts start counting from now
    input_activity(millis());
    set_leds_off(false);
}

void InactivityManager::set_leds_off(bool off)
{
    if (m_leds_off == off)
    {
        return;
    }
    m_leds_off = off;
    if (off)
    {
        ProfileManager::instance().for_each_profile([](uint32_t, const std::shared_ptr<Profile> &profile) {
            for (const auto &led : profile->leds)
            {
                led->off();
            }
        });
        return;
    }
    // Rumble, player and stage kit LEDs only update when the host sends something, so put back what was last sent
    ProfileManager::instance().for_each_instance([](const std::shared_ptr<Instance> &instance) {
        instance->update_feedback(true);
    });
}

void InactivityManager::tick()
{
    uint32_t now = millis();
    // Keep everything awake while the config tool is open, so the LEDs can be previewed
    if (!HIDConfigDevice::tool_closed())
    {
        input_activity(now);
    }
    uint32_t idle = idle_ms(now);
    set_leds_off(m_led_timeout_ms && idle >= m_led_timeout_ms);
    if (m_sleep_timeout_ms && idle >= m_sleep_timeout_ms && !tud_mounted())
    {
        go_to_sleep();
    }
}

void InactivityManager::go_to_sleep()
{
    printf("sleeping until pin %d\r\n", m_wake_pin);
    set_leds_off(true);
    // Push the LEDs being off out to the LED devices, and give them time to send it
    DeviceManager::instance().update(false, false);
    busy_wait_ms(10);
    tud_disconnect();
    if (BluetoothStack::instance().initialized())
    {
        cyw43_arch_deinit();
    }
    gpio_init(m_wake_pin);
    gpio_set_dir(m_wake_pin, GPIO_IN);
    gpio_set_pulls(m_wake_pin, !m_wake_active_high, m_wake_active_high);
    sleep_run_from_xosc();
    sleep_goto_dormant_until_pin(m_wake_pin, true, m_wake_active_high);
    // Everything was shut down to sleep, so start again from scratch
    watchdog_reboot(0, 0, 0);
    while (true)
    {
    }
}
